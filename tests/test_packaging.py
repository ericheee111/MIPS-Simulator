#!/usr/bin/env python3
"""Platform-neutral regressions for packaging helpers, also run on Windows."""
import io
import pathlib
import stat
import sys
import tarfile
import tempfile
import unittest
from unittest import mock
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent/'scripts'))
from package_smoke import isolated_environment, safe_extract, _copy_member
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
                    # Assign after construction: Windows ZipInfo otherwise normalizes '\\'.
                    entry = zipfile.ZipInfo()
                    entry.filename = name
                    package.writestr(entry,'bad')
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


class BoundedCopyTests(unittest.TestCase):
    def test_exact_and_empty_streams(self):
        for size in (0, 1, 65537):
            with self.subTest(size=size):
                sink = io.BytesIO()
                self.assertEqual(_copy_member(io.BytesIO(b'x'*size), sink, size, size), size)
                self.assertEqual(sink.getvalue(), b'x'*size)

    def test_metadata_mismatch_and_truncation(self):
        for content, declared in ((b'ab', 1), (b'a', 2), (b'a', 0)):
            with self.subTest(content=content, declared=declared):
                sink = io.BytesIO()
                with self.assertRaisesRegex(RuntimeError, 'size mismatch'):
                    _copy_member(io.BytesIO(content), sink, declared, 8)
                self.assertLessEqual(len(sink.getvalue()), declared)

    def test_budget_and_oversized_backend_response(self):
        for declared, budget in ((-1, 3), (3, 2), (0, -1)):
            sink = io.BytesIO()
            with self.assertRaisesRegex(RuntimeError, 'resource limit'):
                _copy_member(io.BytesIO(b'abc'), sink, declared, budget)
            self.assertEqual(sink.getvalue(), b'')
        class OversizedResponse:
            def read(self, unused_size):
                return b'four'
        sink = io.BytesIO()
        with self.assertRaisesRegex(RuntimeError, 'resource limit'):
            _copy_member(OversizedResponse(), sink, 3, 3)
        self.assertEqual(sink.getvalue(), b'')

    def test_both_archive_paths_verify_actual_output(self):
        # Inject a tiny inconsistent backend stream. This tests our own bound,
        # not an assertion that the standard library accepts a crafted archive.
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for kind in ('zip', 'tar'):
                archive = root/('test.'+kind)
                if kind == 'zip':
                    with zipfile.ZipFile(archive, 'w') as package:
                        package.writestr('item', 'x')
                    owner, method = zipfile.ZipFile, 'open'
                else:
                    with tarfile.open(archive, 'w') as package:
                        entry = tarfile.TarInfo('item'); entry.size = 1
                        package.addfile(entry, io.BytesIO(b'x'))
                    owner, method = tarfile.TarFile, 'extractfile'
                output = root/kind
                output.mkdir()
                with mock.patch.object(owner, method, return_value=io.BytesIO(b'xx')):
                    with self.assertRaisesRegex(RuntimeError, 'size mismatch'):
                        safe_extract(archive, output, max_bytes=8)
                self.assertEqual((output/'item').stat().st_size, 0)

    def test_archive_budget_and_entry_count_are_cumulative(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for kind in ('zip', 'tar'):
                archive = root/('test.'+kind)
                if kind == 'zip':
                    with zipfile.ZipFile(archive, 'w') as package:
                        package.writestr('first', 'ab')
                        package.writestr('second', 'cd')
                else:
                    with tarfile.open(archive, 'w') as package:
                        for name, content in (('first', b'ab'), ('second', b'cd')):
                            entry = tarfile.TarInfo(name); entry.size = len(content)
                            package.addfile(entry, io.BytesIO(content))
                for number, limits in enumerate(({'max_bytes':3}, {'max_entries':1})):
                    with self.subTest(kind=kind, limits=limits):
                        output = root/(kind+str(number))
                        output.mkdir()
                        with self.assertRaisesRegex(RuntimeError, 'resource limit'):
                            safe_extract(archive, output, **limits)
                        self.assertEqual((output/'first').read_bytes(), b'ab')
                        self.assertFalse((output/'second').exists())

    def test_compressed_and_empty_members_at_exact_output_limit(self):
        payload = b'x'*65537  # Cross the copy-buffer boundary without large fixtures.
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for kind in ('zip', 'tar.gz'):
                archive = root/('test.'+kind)
                if kind == 'zip':
                    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as package:
                        package.writestr('payload', payload)
                        package.writestr('empty', b'')
                else:
                    with tarfile.open(archive, 'w:gz') as package:
                        for name, content in (('payload', payload), ('empty', b'')):
                            entry = tarfile.TarInfo(name); entry.size = len(content)
                            package.addfile(entry, io.BytesIO(content))
                output = root/kind
                output.mkdir()
                safe_extract(archive, output, max_bytes=len(payload), max_entries=2)
                self.assertEqual((output/'payload').read_bytes(), payload)
                self.assertEqual((output/'empty').read_bytes(), b'')


if __name__ == '__main__':
    unittest.main()
