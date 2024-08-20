@echo off
cl /nologo oios.c oi.c trace.c oidis.c /I. /EHsc /DOI8 /DDEBUG /O2 /Oi /Fa /Qpar /Zi /jumptablerdata /Feoios8.exe /link /OPT:REF user32.lib 


