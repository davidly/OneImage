@echo off
setlocal

set outputfile=test_oios.txt
echo %date% %time% >%outputfile%

set _applist=sieveoi eoi tttoi testoi

set _basiclist=e sieve ttt tp texp tcpm tfor tcomp tgosub tmul test tparen tneg tneg1 ta2dim

( for %%a in (%_applist%) do ( call :appRun %%a ) )

( for %%a in (%_basiclist%) do ( call :basicRun %%a ) )

oia -w:2 tttoi >>%outputfile%
oios2 tttoi 17 >>%outputfile%
oia -w:4 tttoi >>%outputfile%
oios4 tttoi 13 >>%outputfile%

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

goto :eof

:basicRun

ba -q -a:o -x basic\%~1
del %~1.s 1>nul 2>nul
move basic\%~1.s . 1>nul

:appRun

echo test %~1
echo test %~1 >>%outputfile%

echo   test %~1 as 2-bytes
echo test %~1 as 2-bytes >>%outputfile%
oia -w:2 %~1.s >>%outputfile%
oios2 %~1 >>%outputfile%

echo   test %~1 as 4-bytes
echo test %~1 as 4-bytes >>%outputfile%
oia -w:4 %~1.s >>%outputfile%
oios4 %~1 >>%outputfile%

echo   test %~1 as 8-bytes
echo test %~1 as 8-bytes >>%outputfile%
oia -w:8 %~1.s >>%outputfile%
oios8 %~1 >>%outputfile%

exit /b 0

:eof



