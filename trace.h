#ifdef NDEBUG

#define trace( x )

#else

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
#define trace_binary_data( p, length, indent )
extern void close_trace( void );

#else

extern void cdecl trace( const char * format, ... );
extern void cdecl trace_binary_data( char * pData, size_t length, size_t indent );
extern void enable_trace( const char * filename );
extern void close_trace( void );

#endif

#endif
