#!/usr/bin/env python3
"""Real executable integration tests with bounded subprocess lifetimes (no sleeps)."""
import pathlib
import subprocess
import sys
import tempfile
import unittest
BINARY = sys.argv[1]
sys.argv = sys.argv[:1]
class CLI(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.path = pathlib.Path(self.temp.name)/'sample.asm'
        self.path.write_text('.text\nmain:\nli $t0, 7\nend:\nj end\n')
    def tearDown(self):
        self.temp.cleanup()
    def invoke(self, commands='', args=None):
        return subprocess.run([BINARY]+(args if args is not None else [str(self.path)]),
                              input=commands, text=True, capture_output=True, timeout=5)
    def test_step_and_initial_state(self):
        p = self.invoke('print $pc\nprint $t0\nstep\nprint $t0\nquit\n')
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stderr, '')
        self.assertIn('simmips> 0x00000000\nsimmips> 0x00000000\nsimmips> 0x00000001\nsimmips> 0x00000007\n',p.stdout)
    def test_adjacent_comment_and_signed_immediate(self):
        self.path.write_text('.text\nmain:\nli $t0, -1# comment\nli $t1, +7   ')
        p = self.invoke('step\nstep\nprint $t0\nprint $t1\nquit\n')
        self.assertEqual(p.returncode,0,p.stderr); self.assertEqual(p.stderr,'')
        self.assertIn('0xffffffff',p.stdout); self.assertIn('0x00000007',p.stdout)
    def test_eof_idle(self):
        self.assertEqual(self.invoke().returncode,0)
    def test_eof_running(self):
        self.assertEqual(self.invoke('run\n').returncode,0)
    def test_break_then_quit(self):
        for _ in range(20):
            p = self.invoke('run\nbreak\nquit\n')
            self.assertEqual(p.returncode,0,p.stderr)
    def test_running_restrictions(self):
        p=self.invoke('run\nstep\nprint $t0\nbreak\nquit\n')
        self.assertEqual(p.returncode,0)
        self.assertEqual(p.stderr,'Error: simulation running. Type break to halt.\n'*2)
    def test_invalid_print_never_aborts(self):
        p=self.invoke('print\nprint $\nprint &0x\nprint &0xffffffffffffffffffff\nprint &0xffffffff\nquit\n')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(p.stderr.count('Error:'),5)
    def test_missing_file(self):
        p=self.invoke(args=[str(self.path)+'missing'])
        self.assertNotEqual(p.returncode,0); self.assertIn('Error:1:',p.stderr)
    def test_bad_options(self):
        for args in ([],['--gui'],['--bad'],[str(self.path),'extra']):
            self.assertNotEqual(self.invoke(args=args).returncode,0)
    def test_help(self):
        p=self.invoke(args=['--help']); self.assertEqual(p.returncode,0); self.assertIn('Usage:',p.stdout)
    def test_bad_source_line(self):
        self.path.write_text('# comment\n.text\nmain:\nli $t0, +\n')
        p=self.invoke(); self.assertNotEqual(p.returncode,0); self.assertIn('Error:4:',p.stderr)
    def test_no_main(self):
        self.path.write_text('.text\nnop\n')
        p=self.invoke(); self.assertNotEqual(p.returncode,0); self.assertIn('main',p.stderr)
    def test_memory_boundary_fault(self):
        self.path.write_text('.text\nmain:\nli $t0, 7\nsw $t0, 1021\n')
        p=self.invoke('step\nstep\nstatus\nquit\n')
        self.assertEqual(p.returncode,0); self.assertIn('out of bounds',p.stderr)
if __name__=='__main__': unittest.main()
