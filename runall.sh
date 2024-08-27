#!/bin/bash

outputfile="test_oios.txt"
echo $(date) >$outputfile

declare -a _applist=( sieveoi eoi tttoi testoi )
declare -a _basiclist=( e sieve ttt tp texp tcpm tfor tcomp )

test_app()
{
    echo test $1
    echo test $1 >>$outputfile

    echo   test $1 as 2-bytes
    echo test $1 as 2-bytes >>$outputfile
    oia -w:2 $1.s >>$outputfile
    oios2 $1 >>$outputfile

    echo   test $1 as 4-bytes
    echo test $1 as 4-bytes >>$outputfile
    oia -w:4 $1.s >>$outputfile
    oios4 $1 >>$outputfile

    echo   test $1 as 8-bytes
    echo test $1 as 8-bytes >>$outputfile
    oia -w:8 $1.s >>$outputfile
    oios8 $1 >>$outputfile
}

test_basic_app()
{
    ba -q -a:o -x $1
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

echo $(date) >>$outputfile
diff -i -B -w baseline_$outputfile $outputfile
