@echo off
setlocal
cd /d "%~dp0"
set "PATH=%CD%;%PATH%"

set outputfile=test_oios.txt
echo OneImage regression tests >%outputfile%

set _applist=sieveoi eoi tttoi testoi tests/isa_arithmetic tests/isa_relations tests/isa_memory

set _basiclist=e sieve ttt tp texp tcpm tfor tcomp tgosub tmul test tparen tneg tneg1 ta2dim

( for %%a in (%_applist%) do ( call :appRun %%a ) )

( for %%a in (%_basiclist%) do ( call :basicRun %%a ) )

oia -w:2 tttoi >>%outputfile%
oios2 tttoi 17 >>%outputfile%
oia -w:4 tttoi >>%outputfile%
oios4 tttoi 13 >>%outputfile%

echo OneImage regression tests complete >>%outputfile%
python tests\compare_baseline.py baseline_%outputfile% %outputfile%
exit /b %errorlevel%

:basicRun

"%~dp0ba.exe" -q -a:o -x basic\%~1
del %~1.s 1>nul 2>nul
move basic\%~1.s . 1>nul

:appRun

echo test %~1
echo test %~1 >>%outputfile%

echo   test %~1 as 2-bytes
echo test %~1 as 2-bytes >>%outputfile%
oia -w:2 %~1.s >>%outputfile%
oios2 %~1 >>%outputfile%
oios4 %~1 >>%outputfile%
oios8 %~1 >>%outputfile%
rem rvos ..\rvos\debianrv\oios2 %~1 >>%outputfile%

echo   test %~1 as 4-bytes
echo test %~1 as 4-bytes >>%outputfile%
oia -w:4 %~1.s >>%outputfile%
oios4 %~1 >>%outputfile%
oios8 %~1 >>%outputfile%
rem rvos ..\rvos\debianrv\oios4 %~1 >>%outputfile%

echo   test %~1 as 8-bytes
echo test %~1 as 8-bytes >>%outputfile%
oia -w:8 %~1.s >>%outputfile%
oios8 %~1 >>%outputfile%
rem rvos ..\rvos\debianrv\oios8 %~1 >>%outputfile%

exit /b 0

:eof



