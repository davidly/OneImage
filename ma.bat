@echo off
cl /nologo oia.c oidis.c /I. /EHsc /DOIOS_WIDE /DOIOS_64 /DDEBUG /O2 /Oi /Fa /Qpar /Zi /jumptablerdata /link /OPT:REF user32.lib 


