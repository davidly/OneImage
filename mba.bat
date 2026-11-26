@echo off
setlocal
pushd "%~dp0"
cl /nologo /O2 /EHsc /DNDEBUG ba.cxx /Fe:ba.exe
set "result=%errorlevel%"
popd
exit /b %result%
