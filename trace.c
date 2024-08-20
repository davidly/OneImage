#include "trace.h"

#ifndef NDEBUG
static FILE * fp = 0;

#ifdef OLDCPU
void enable_trace( filename ) char * filename;
#else
void enable_trace( const char * filename )
#endif
{
    close_trace();
    fp = fopen( filename, "w+t" );
}

void close_trace()
{
    if ( 0 != fp )
    {
        fclose( fp );
        fp = 0;
    }
}

#ifndef OLDCPU
void cdecl trace( const char * format, ... )
{
    if ( 0 != fp )
    {
        va_list args;
        va_start( args, format );
        vfprintf( fp, format, args );
        va_end( args );
        fflush( fp );
    }
}
#endif

#endif

