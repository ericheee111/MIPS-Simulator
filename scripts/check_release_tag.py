#!/usr/bin/env python3
"""Reject release tags that do not match the CMake project version."""
import pathlib
import re
import sys


def validate(tag, cmake):
    match = re.search(r"project\(MIPSSimulator\s+VERSION\s+(\d+\.\d+\.\d+)\b", cmake)
    if not match or tag != 'v'+match.group(1):
        raise ValueError('Release tag must match v<PROJECT_VERSION> from CMakeLists.txt')
    return match.group(1)


if __name__ == '__main__':
    source = pathlib.Path(__file__).resolve().parent.parent / 'CMakeLists.txt'
    print(validate(sys.argv[1], source.read_text(encoding='utf-8')))
