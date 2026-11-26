#!/bin/sh
set -eu
cd "$(dirname "$0")"
"${CXX:-c++}" -std=c++11 -O3 -DNDEBUG -DFORCETRACING -I . oia.c oidis.c -o oia
