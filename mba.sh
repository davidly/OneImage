#!/bin/sh
set -eu
cd "$(dirname "$0")"
"${CXX:-c++}" -std=c++11 -O2 -DNDEBUG ba.cxx -o ba
