#ifndef MSC6
#include <stdint.h>
#endif

#include <string.h>

#include "oi.h"
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

static char * appendHexNibble( char * p, char val )
{
    *p++ = ( val <= (char) 9 ) ? val + (char) '0' : val - (char) 10 + (char) 'a';
    return p;
} //appendHexNibble

static char * appendHexByte( char * p, char val )
{
    p = appendHexNibble( p, (char) ( ( val >> 4 ) & 0xf ) );
    p = appendHexNibble( p, (char) ( val & 0xf ) );
    return p;
} //appendBexByte

static char * appendHexWord( char * p, size_t val )
{
    p = appendHexByte( p, (char) ( ( val >> 8 ) & 0xff ) );
    p = appendHexByte( p, (char) ( val & 0xff ) );
    return p;
} //appendHexWord

#define bytesPerRow 32

void cdecl trace_binary_data( char * pData, size_t length, size_t indent )
{
    size_t offset = 0;
    size_t beyond = length;
    char ch, buf[ bytesPerRow ];
    size_t i, o, sp, extraSpace;
    size_t end_of_row, cap, toread, spaceNeeded;
    static char acLine[ 200 ];
    
    while ( offset < beyond )
    {
        char * pline = acLine;
    
        for ( i = 0; i < indent; i++ )
            *pline++ = ' ';
    
        pline = appendHexWord( pline, (uint16_t) offset );
        *pline++ = ' ';
        *pline++ = ' ';

        end_of_row = offset + bytesPerRow;
        cap = ( end_of_row > beyond ) ? beyond : end_of_row;
        toread = ( ( offset + bytesPerRow ) > beyond ) ? ( length % bytesPerRow ) : bytesPerRow;
    
        memcpy( buf, pData + offset, toread );

        extraSpace = 2;
    
        for ( o = offset; o < cap; o++ )
        {
            pline = appendHexByte( pline, buf[ o - offset ] );
            *pline++ = ' ';
            if ( ( bytesPerRow > 16 ) && ( o == ( offset + 15 ) ) )
            {
                *pline++ = ':';
                *pline++ = ' ';
                extraSpace = 0;
            }
        }
    
        spaceNeeded = extraSpace + ( ( bytesPerRow - ( cap - offset ) ) * 3 );
    
        for ( sp = 0; sp < ( (size_t) 1 + spaceNeeded ); sp++ )
            *pline++ = ' ';
    
        for ( o = offset; o < cap; o++ )
        {
            ch = buf[ o - offset ];
    
            if ( (int8_t) ch < ' ' || 127 == ch )
                ch = '.';
    
            *pline++ = ch;
        }
    
        offset += bytesPerRow;
        *pline = 0;

        trace( "%s\n", acLine );
    }
} /* trace_binary_data */

#endif

#endif

