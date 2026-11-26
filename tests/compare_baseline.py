#!/usr/bin/env python3
"""Compare regression output across platforms; requires Python 3.7+."""
import argparse
import difflib
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('actual', type=Path)
    args = parser.parse_args()
    try:
        # CMD echo adds trailing spaces before redirection. Ignore those and
        # platform line endings while preserving case, blank lines, and values.
        expected = [line.rstrip(' \t') for line in args.baseline.read_text().splitlines()]
        actual = [line.rstrip(' \t') for line in args.actual.read_text().splitlines()]
    except OSError as error:
        print(str(error), file=sys.stderr)
        return 1
    if actual != expected:
        print('\n'.join(difflib.unified_diff(expected, actual,
              fromfile=str(args.baseline), tofile=str(args.actual), lineterm='')))
        return 1
    print('Regression output matches baseline.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
