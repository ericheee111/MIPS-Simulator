#!/usr/bin/env python3
"""Run the real install script with a fake deploy tool; no Qt/Windows required.

This tests selection, installation and fail-closed policy, not DLL ABI or GUI
launch. The Windows CI job separately builds and launches the real Qt package.
"""
import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE = pathlib.Path(__file__).resolve().parent.parent
CMAKE = shutil.which('cmake')


class QtDeploymentTests(unittest.TestCase):
    def deploy(self, config='Release', missing_plugin=False, missing_license=False,
               deploy_result='0', deploy_windows=True):
        temporary = tempfile.TemporaryDirectory(prefix='mips qt deployment ')
        self.addCleanup(temporary.cleanup)
        root = pathlib.Path(temporary.name)
        plugins = root/'selected Qt'/'plugins'
        (plugins/'platforms').mkdir(parents=True)
        suffix = 'd' if config == 'Debug' else ''
        for variant in ('', 'd'):
            if not (missing_plugin and variant == suffix):
                (plugins/'platforms'/f'qoffscreen{variant}.dll').write_text('selected '+variant)
        licenses = root/'licenses'
        licenses.mkdir()
        for name in ('LICENSE.LGPLv3', 'LICENSE.GPL3', 'LICENSE.FDL'):
            if not (missing_license and name == 'LICENSE.LGPLv3'):
                (licenses/name).write_text('test license fixture')
        prefix = root/'installed tree'
        (prefix/'bin').mkdir(parents=True)
        (prefix/'bin'/'simmips.exe').write_text('test executable fixture')
        tool = root/'selected Qt'/'bin'/'windeployqt.exe'
        settings = {
            'CMAKE_INSTALL_BINDIR': 'bin', 'CMAKE_INSTALL_DATADIR': 'share',
            'MIPS_QT_PLUGINS_DIR': plugins.as_posix(),
            'MIPS_QT_LICENSE_DIR': licenses.as_posix(),
            'MIPS_WINDEPLOYQT': tool.as_posix(),
        }
        text = (SOURCE/'cmake'/'DeployQt.cmake.in').read_text(encoding='utf-8')
        for name, value in settings.items():
            text = text.replace('@'+name+'@', value)
        deploy = root/'DeployQt.cmake'
        deploy.write_text(text, encoding='utf-8')
        installed_plugin = (prefix/'bin'/'platforms'/f'qoffscreen{suffix}.dll').as_posix()
        windows = (prefix/'bin'/'platforms'/f'qwindows{suffix}.dll').as_posix()
        flag = '--debug' if config == 'Debug' else '--release'
        fake_windows = f'file(WRITE "{windows}" "native backend fixture")' if deploy_windows else ''
        driver = root/'driver.cmake'
        driver.write_text(f'''cmake_minimum_required(VERSION 3.16)
set(CMAKE_INSTALL_PREFIX "{prefix.as_posix()}")
set(CMAKE_INSTALL_CONFIG_NAME "{config}")
function(execute_process)
  if(NOT "${{ARGV1}}" STREQUAL "{tool.as_posix()}")
    message(FATAL_ERROR "unexpected Qt tool")
  endif()
  list(FIND ARGV "{flag}" configuration_index)
  list(FIND ARGV "{installed_plugin}" plugin_index)
  if(configuration_index LESS 0 OR plugin_index LESS 0 OR NOT EXISTS "{installed_plugin}")
    message(FATAL_ERROR "offscreen dependency input/configuration missing")
  endif()
  {fake_windows}
  list(FIND ARGV RESULT_VARIABLE result_index)
  math(EXPR result_index "${{result_index}}+1")
  list(GET ARGV ${{result_index}} result_name)
  set(${{result_name}} "{deploy_result}" PARENT_SCOPE)
endfunction()
include("{deploy.as_posix()}")
''', encoding='utf-8')
        env = dict(os.environ)
        env.pop('DESTDIR', None)
        result = subprocess.run([CMAKE, '-P', str(driver)], capture_output=True,
                                text=True, timeout=15, env=env)
        return result, prefix, suffix

    def test_configuration_and_relocated_prefix(self):
        for config in ('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel'):
            with self.subTest(config=config):
                result, prefix, suffix = self.deploy(config)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                platform_dir = prefix/'bin'/'platforms'
                self.assertEqual((platform_dir/f'qoffscreen{suffix}.dll').read_text(), 'selected '+suffix)
                self.assertTrue((platform_dir/f'qwindows{suffix}.dll').is_file())
                self.assertEqual((prefix/'bin'/'qt.conf').read_text(), '[Paths]\nPrefix=.\nPlugins=.\n')
                for name in ('LICENSE.LGPLv3', 'LICENSE.GPL3', 'LICENSE.FDL'):
                    self.assertTrue((prefix/'share'/'mips-simulator'/'licenses'/'Qt5'/name).is_file())

    def test_missing_matching_plugin_is_not_replaced_by_other_configuration(self):
        for config in ('Release', 'Debug'):
            with self.subTest(config=config):
                result, _, _ = self.deploy(config, missing_plugin=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Missing required Qt offscreen plugin', result.stderr)

    def test_missing_deployed_native_backend_fails(self):
        result, _, _ = self.deploy(deploy_windows=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Missing deployed Qt platform plugin', result.stderr)

    def test_deploy_tool_failure_and_launch_error_are_not_success(self):
        for error in ('1', 'No such file or directory'):
            with self.subTest(error=error):
                result, _, _ = self.deploy(deploy_result=error)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('windeployqt failed', result.stderr)

    def test_missing_upstream_license_fails(self):
        result, _, _ = self.deploy(missing_license=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Missing Qt license file LICENSE.LGPLv3', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--cmake', default=CMAKE)
    args, remaining = parser.parse_known_args()
    CMAKE = args.cmake
    if not CMAKE:
        parser.error('cmake is required for the deployment contract tests')
    unittest.main(argv=[sys.argv[0], *remaining])
