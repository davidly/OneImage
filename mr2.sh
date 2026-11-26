#!/bin/sh
set -eu
cd "$(dirname "$0")"
"${CXX:-c++}" -std=c++11 -O3 -DNDEBUG -DOI2 -I . -x c++ oios.c oi.c trace.c oidis.c -o oios2
