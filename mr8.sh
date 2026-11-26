#!/bin/sh
set -eu
cd "$(dirname "$0")"
"${CXX:-c++}" -std=c++11 -O3 -DNDEBUG -DOI8 -I . oios.c oi.c trace.c oidis.c -o oios8
