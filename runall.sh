#!/bin/bash
set -e
cd "$(dirname "$0")"
export PATH="$PWD:$PATH"

outputfile="test_oios.txt"
echo "OneImage regression tests" >$outputfile

declare -a _applist=(
    sieveoi eoi tttoi testoi
    tests/isa_arithmetic tests/isa_relations tests/isa_memory
)
declare -a _basiclist=( e sieve ttt tp texp tcpm tfor tcomp tgosub tmul test tparen tneg tneg1 ta2dim )

test_app()
{
    echo test $1
    echo test $1 >>$outputfile

    echo   test $1 as 2-bytes
    echo test $1 as 2-bytes >>$outputfile
    oia -w:2 $1.s >>$outputfile
    oios2 $1 >>$outputfile
    oios4 $1 >>$outputfile
    oios8 $1 >>$outputfile

    echo   test $1 as 4-bytes
    echo test $1 as 4-bytes >>$outputfile
    oia -w:4 $1.s >>$outputfile
    oios4 $1 >>$outputfile
    oios8 $1 >>$outputfile

    echo   test $1 as 8-bytes
    echo test $1 as 8-bytes >>$outputfile
    oia -w:8 $1.s >>$outputfile
    oios8 $1 >>$outputfile
}

test_basic_app()
{
    ./ba -q -a:o -x basic/$1
    rm -f $1.s
    mv basic/$1.s .
    test_app $1
}

for app in ${_applist[*]}
do
    test_app $app
done

for app in ${_basiclist[*]}
do
    test_basic_app $app
done

oia -w:2 tttoi >>$outputfile
oios2 tttoi 17 >>$outputfile
oia -w:4 tttoi >>$outputfile
oios4 tttoi 13 >>$outputfile

echo "OneImage regression tests complete" >>$outputfile
python3 tests/compare_baseline.py baseline_$outputfile $outputfile
