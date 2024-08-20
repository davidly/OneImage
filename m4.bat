@echo off
cl /nologo oios.c oi.c trace.c oidis.c /I. /EHsc /DOI4 /DDEBUG /O2 /Oi /Fa /Qpar /Zi /jumptablerdata /Feoios4.exe /link /OPT:REF user32.lib 


