#!/bin/sh
set -eu
cd "$(dirname "$0")"
# oia.c and oidis.c must also build with CP/M-era C compilers that lack snprintf,
# so silence clang/macOS sprintf deprecation warnings rather than rewriting the calls.
"${CXX:-c++}" -std=c++11 -O3 -DNDEBUG -DFORCETRACING -Wno-deprecated-declarations -I . -x c++ oia.c oidis.c -o oia
