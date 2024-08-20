@echo off
del *.obj
ntvdm -c -r:.. -e:path=c:\bin;c:\binb,include=c:\include;.,lib=c:\lib ..\bin\cl.exe /DOI2 /DMSC6 /DNDEBUG /W3 /nologo /Zp1 /AS /B1 C1L /B2 C2L /Gr /Owx /Feoios2dos.exe /Fcoios.cod oios.c oi.c oidis.c trace.c

