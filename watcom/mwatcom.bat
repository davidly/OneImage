@echo off
setlocal
SET WATCOM=..\WATCOM
SET PATH=%WATCOM%\BINNT64;%WATCOM%\BINNT;%PATH%
SET EDPATH=%WATCOM%\EDDAT
SET INCLUDE=%WATCOM%\H;%WATCOM%\H\NT;..\djl

wcl -d0 -cc -q -zp=1 -ms -obmr -oh -ei -oi -s -0 -j -oe=160 -ol+ -ot oios.c oi.c oidis.c -bcl=DOS -k8192 -fe=oioswdos.exe -DOI2 -DWATCOM -DNDEBUG -lr

rem wcl -?




