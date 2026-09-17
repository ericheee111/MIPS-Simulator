#!/usr/bin/env python3
"""Check the installed tree, with developer/Qt paths removed on Windows."""
import argparse
import os
import pathlib
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('prefix', type=pathlib.Path)
    parser.add_argument('--gui', action='store_true')
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    binary = prefix / 'bin' / ('simmips.exe' if os.name == 'nt' else 'simmips')
    example = prefix / 'share' / 'mips-simulator' / 'examples' / 'sum_of_squares.asm'
    env = os.environ.copy()
    for name in ('QT_PLUGIN_PATH', 'QML2_IMPORT_PATH', 'QTDIR', 'QT_QPA_PLATFORM_PLUGIN_PATH'):
        env.pop(name, None)
    if os.name == 'nt':
        windows = pathlib.Path(env['SystemRoot'])
        env['PATH'] = os.pathsep.join(map(str, (windows/'System32', windows)))
    env['QT_QPA_PLATFORM'] = 'offscreen'
    version = subprocess.run([str(binary), '--version'], env=env, capture_output=True, text=True, timeout=15, check=True)
    assert '1.1.0' in version.stdout, version.stdout
    cli = subprocess.run([str(binary), str(example)], input='until end\nprint &0x4\nprint &0x5\nquit\n',
                         env=env, capture_output=True, text=True, timeout=15, check=True)
    assert not cli.stderr, cli.stderr
    assert '0x81' in cli.stdout and '0x01' in cli.stdout, cli.stdout
    if args.gui:
        if os.name == 'nt':
            assert (prefix/'bin'/'platforms'/'qoffscreen.dll').is_file()
        subprocess.run([str(binary), '--gui', str(example), '--smoke-test'], env=env,
                       capture_output=True, text=True, timeout=15, check=True)
    print('Installed package passed: version, deterministic CLI' + (', Qt launch/close with isolated paths' if args.gui else ''))


if __name__ == '__main__':
    main()
