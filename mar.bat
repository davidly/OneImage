@echo off
cl /W4 /wd4702 /wd4996 /nologo /jumptablerdata /I. /EHsc /DOIOS_WIDE /DOIOS_64 /DFORCETRACING /DNDEBUG /GS- /GL /Ot /Ox /Ob3 /Oi /Qpar /Zi /Fa /FAsc oia.c oidis.c /link /OPT:REF user32.lib

