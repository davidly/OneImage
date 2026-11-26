#!/bin/sh
set -eu
cd "$(dirname "$0")"
for script in mba.sh mar.sh mr2.sh mr4.sh mr8.sh; do
    echo "=== $script ==="
    "./$script"
done
