@echo off
cl /W4 /wd4206 /wd4702 /wd4996 /nologo /jumptablerdata /I. /EHsc /DOI4 /DNDEBUG /GS- /GL /Ot /Ox /Ob3 /Oi /Qpar /Zi /Fa /FAsc oios.c oi.c trace.c oidis.c /Feoios4.exe /link /OPT:REF user32.lib

