#!/usr/bin/env python3
"""Reconstruct missing VM fixtures from assembly literals in the PUBLIC legacy tests.
This does not claim to recover the original private instructor fixtures.
"""
import argparse
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[1]
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    source = (ROOT/'test_parser.cpp').read_text()
    cases = list(re.finditer(r'TEST_CASE\("vm(\d+)"\)', source))
    for i, case in enumerate(cases):
        block = source[case.end():cases[i+1].start() if i+1 < len(cases) else len(source)]
        assembly = re.search(r'std::string file = R"\((.*?)\)";', block, re.S)
        if not assembly:
            raise RuntimeError('No assembly in '+case.group(0))
        expected = assembly.group(1)
        path = ROOT/'tests'/'vm'/('test'+case.group(1)+'.asm')
        if args.check:
            if not path.exists() or path.read_bytes() != expected.encode():
                raise RuntimeError('Fixture differs from public test: '+str(path))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(expected.encode())
    if len(cases) != 21:
        raise RuntimeError('Expected 21 public VM fixtures')
    print('21 fixtures match public assembly literals')
if __name__ == '__main__':
    main()
