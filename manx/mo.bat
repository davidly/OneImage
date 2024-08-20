@echo off

del oios.o 2>nul
del oi.o 2>nul
del oios.com 2>nul

ntvcm ..\cc -DOI2 -DNDEBUG -DOLDCPU -DAZTECCPM -Q -T -F oios.c
ntvcm ..\cc -DOI2 -DNDEBUG -DOLDCPU -DAZTECCPM -Y256 -Q -T -F oi.c
ntvcm ..\as -L oios.asm
ntvcm ..\as -L oi.asm
ntvcm ..\ln -T oios.o oi.o m.lib c.lib
goto :done

:done

