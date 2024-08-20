@echo off
del *.obj
ntvdm -c -r:.. -e:path=c:\bin;c:\binb,include=c:\include;.,lib=c:\lib ..\bin\cl.exe /DOI2 /DMSC6 /W3 /nologo /AS /B1 C1L /B2 C2L /Gr /Os /Gs /Feoiados.exe oia.c oidis.c trace.c


