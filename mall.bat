@echo off
setlocal
pushd "%~dp0"
for %%s in (mba.bat mar.bat mr2.bat mr4.bat mr8.bat) do (
    echo === %%s ===
    call %%s
    if errorlevel 1 goto failed
)
popd
exit /b 0
:failed
popd
exit /b 1
