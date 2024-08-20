#ifndef NDEBUG

#include <stdio.h>
#include <stdlib.h>

#ifndef HISOFTCPM
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#endif

#ifdef OLDCPU

#define trace printf
extern void enable_trace( char * );
extern void close_trace( void );

#else

extern void cdecl trace( const char * format, ... );
extern void enable_trace( const char * filename );
extern void close_trace( void );

#endif

#endif
