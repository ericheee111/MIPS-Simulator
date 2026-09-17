#!/usr/bin/env python3
"""Verify installed trees and extracted packages without developer/Qt search paths."""
import argparse
import os
import pathlib
import shutil
import stat
import subprocess
import tarfile
import tempfile
import zipfile


def isolated_environment(source, windows=False):
    # os.environ on Windows is case-insensitive; its plain-dict copy is not.
    env = {(key.upper() if windows else key): value for key, value in source.items()}
    for key in list(env):
        if key.upper().startswith(('QT_', 'QML')) or key.upper() in ('QTDIR', 'LD_LIBRARY_PATH'):
            del env[key]
    if windows:
        root = env.get('SYSTEMROOT')
        if not root:
            raise RuntimeError('SYSTEMROOT is required for a Windows package check')
        env['PATH'] = ';'.join((str(pathlib.PureWindowsPath(root)/'System32'), root))
    return env


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def safe_extract(archive, destination):
    """Only regular files/directories under a fresh destination; bounded archives."""
    total = 0
    count = 0

    def target(name, size):
        nonlocal total, count
        path = pathlib.PurePosixPath(name)
        require(not path.is_absolute() and '..' not in path.parts and
                '\\' not in name and ':' not in name, 'unsafe archive path: '+name)
        total += size
        count += 1
        require(total <= 512*1024*1024 and count <= 10000, 'archive resource limit exceeded')
        result = destination.joinpath(*path.parts)
        result.parent.mkdir(parents=True, exist_ok=True)
        return result

    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as package:
            for entry in package.infolist():
                require(not stat.S_ISLNK(entry.external_attr >> 16), 'archive links are not allowed')
                output = target(entry.filename, entry.file_size)
                if entry.is_dir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    with package.open(entry) as source, output.open('wb') as sink:
                        shutil.copyfileobj(source, sink)
    else:
        with tarfile.open(archive) as package:
            for entry in package:
                require(entry.isdir() or entry.isfile(), 'archive links/devices are not allowed')
                output = target(entry.name, entry.size)
                if entry.isdir():
                    output.mkdir(parents=True, exist_ok=True)
                else:
                    with package.extractfile(entry) as source, output.open('wb') as sink:
                        shutil.copyfileobj(source, sink)
                    output.chmod(entry.mode & 0o777)


def verify(prefix, gui=False):
    prefix = prefix.resolve()
    binary = prefix / 'bin' / ('simmips.exe' if os.name == 'nt' else 'simmips')
    example = prefix / 'share' / 'mips-simulator' / 'examples' / 'sum_of_squares.asm'
    require(binary.is_file() and example.is_file(), 'package is missing its executable/example')
    env = isolated_environment(os.environ, os.name == 'nt')
    if os.name == 'nt':
        require(any((prefix/'bin').glob('vcruntime140*.dll')), 'missing app-local MSVC runtime')
        require((prefix/'bin'/'msvcp140.dll').is_file(), 'missing app-local C++ runtime')
    # A fresh working directory prevents loading DLLs/resources from the checkout.
    with tempfile.TemporaryDirectory() as workdir:
        def run(arguments, source=None):
            result = subprocess.run([str(binary), *arguments], input=source, cwd=workdir,
                                    env=env, capture_output=True, text=True, timeout=20)
            require(result.returncode == 0,
                    f'{arguments}: exit={result.returncode}\n{result.stdout}\n{result.stderr}')
            return result
        version = run(['--version'])
        require('MIPS Simulator 1.1.0' in version.stdout, version.stdout)
        cli = run([str(example)], 'until end\nprint &0x4\nprint &0x5\nquit\n')
        require(not cli.stderr and '0x81' in cli.stdout and '0x01' in cli.stdout,
                cli.stdout+'\n'+cli.stderr)
        if gui:
            if os.name == 'nt':
                for plugin in ('qoffscreen.dll', 'qwindows.dll'):
                    require((prefix/'bin'/'platforms'/plugin).is_file(), 'missing '+plugin)
            for platform in (('offscreen', 'windows') if os.name == 'nt' else ('offscreen',)):
                env['QT_QPA_PLATFORM'] = platform
                run(['--gui', str(example), '--smoke-test'])
    print('Package passed: '+str(prefix)+' (version, deterministic CLI'+
          (', isolated Qt launch/close' if gui else '')+')')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('prefix', type=pathlib.Path)
    parser.add_argument('--gui', action='store_true')
    parser.add_argument('--archives', action='store_true', help='Check every archive in the given directory')
    args = parser.parse_args()
    if not args.archives:
        verify(args.prefix, args.gui)
        return
    archives = sorted([*args.prefix.glob('*.zip'), *args.prefix.glob('*.tar.gz')])
    require(bool(archives), 'no package archives found')
    for archive in archives:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            safe_extract(archive, root)
            binaries = list(root.rglob('simmips.exe' if os.name == 'nt' else 'simmips'))
            require(len(binaries) == 1, 'archive must contain exactly one simmips executable')
            verify(binaries[0].parent.parent, args.gui)
        print('Extracted archive verified: '+archive.name)


if __name__ == '__main__':
    main()
