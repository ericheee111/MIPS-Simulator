#!/usr/bin/env python3
"""Platform-neutral regressions for packaging helpers, also run on Windows."""
import io
import pathlib
import stat
import sys
import tarfile
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent/'scripts'))
from package_smoke import isolated_environment, safe_extract
from check_release_tag import validate


class PackagingTests(unittest.TestCase):
    def test_windows_environment_case(self):
        for key in ('SYSTEMROOT', 'SystemRoot', 'systemroot'):
            with self.subTest(key=key):
                env = isolated_environment({key:r'C:\Windows', 'Path':'developer',
                       'Qt_Plugin_Path':'dev', 'QML2_IMPORT_PATH':'dev'}, windows=True)
                self.assertEqual(env['PATH'], r'C:\Windows\System32;C:\Windows')
                self.assertNotIn('QT_PLUGIN_PATH', env)
                self.assertNotIn('QML2_IMPORT_PATH', env)
        with self.assertRaises(RuntimeError):
            isolated_environment({}, windows=True)

    def test_unix_environment_is_not_uppercased(self):
        self.assertEqual(isolated_environment({'PATH':'/usr/bin', 'HOME':'/home/test',
                         'LD_LIBRARY_PATH':'dev'}), {'PATH':'/usr/bin', 'HOME':'/home/test'})

    def test_release_version(self):
        source = 'project(MIPSSimulator VERSION 1.1.0 LANGUAGES CXX)'
        self.assertEqual(validate('v1.1.0', source), '1.1.0')
        for tag in ('v2.0.0', 'v1.1.0-extra', 'v1.1.0\n', '1.1.0'):
            with self.assertRaises(ValueError):
                validate(tag, source)
        with self.assertRaises(ValueError):
            validate('v1.1.0', '')

    def test_regular_zip_and_tar(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for kind in ('zip','tar'):
                archive = root/('test.'+kind)
                if kind == 'zip':
                    with zipfile.ZipFile(archive,'w') as package:
                        package.writestr('pkg/bin/test','content')
                else:
                    with tarfile.open(archive,'w') as package:
                        entry = tarfile.TarInfo('pkg/bin/test'); entry.size=7; entry.mode=0o755
                        package.addfile(entry,io.BytesIO(b'content'))
                output = root/('output-'+kind)
                output.mkdir()
                safe_extract(archive,output)
                self.assertEqual((output/'pkg/bin/test').read_text(),'content')

    def test_zip_traversal_and_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=pathlib.Path(temporary); archive=root/'bad.zip'; output=root/'out'; output.mkdir()
            for name in ('../outside','/absolute','C:/drive','pkg\\escape'):
                with zipfile.ZipFile(archive,'w') as package:
                    package.writestr(name,'bad')
                with self.assertRaises(RuntimeError):
                    safe_extract(archive,output)
            with zipfile.ZipFile(archive,'w') as package:
                entry=zipfile.ZipInfo('link'); entry.external_attr=(stat.S_IFLNK | 0o777)<<16
                package.writestr(entry,'../outside')
            with self.assertRaises(RuntimeError):
                safe_extract(archive,output)

    def test_tar_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=pathlib.Path(temporary); archive=root/'bad.tar'; output=root/'out'; output.mkdir()
            with tarfile.open(archive,'w') as package:
                entry=tarfile.TarInfo('link'); entry.type=tarfile.SYMTYPE; entry.linkname='../outside'
                package.addfile(entry)
            with self.assertRaises(RuntimeError):
                safe_extract(archive,output)


if __name__ == '__main__':
    unittest.main()
