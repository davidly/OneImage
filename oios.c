#include <stdio.h>

#ifndef AZTECCPM

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef MSC6
#include <stdint.h>
#endif

#else

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

char * strchr( pc, c ) char * pc; char c;
{
    while ( *pc )
    {
        if ( c == *pc )
            return pc;
        pc++;
    }
    return 0;
} /* strchr */

#endif

#include "oi.h"
#include "oios.h"
#include "trace.h"

#define true 1
#define false 0

int g_halted = 0;
uint8_t * ram = 0;

void OIHardTermination()
{
    exit( 1 );
} /* OIHardTermination */

#ifdef OLDCPU
void OISyscall( function ) size_t function;
#else
void OISyscall( size_t function )
#endif
{
    switch( function )
    {
        case 0:
        {
            /* this results in a halt instruction, which calls OIHalt(), which sets g_halted = true */
            g_oi.rpc = 0;
            break;
        }
        case 1:
        {
            printf( "%s", ram + g_oi.rarg1 );
#ifndef NDEBUG
            trace( "syscall string: %s\n", ram + g_oi.rarg1 );
#endif
            break;
        }
        case 2:
        {
#ifdef OI2
            printf( "%d", (int16_t) g_oi.rarg1 );
#ifndef NDEBUG
            trace( "syscall integer: %d\n", (int16_t) g_oi.rarg1 );
#endif
#endif

#ifdef OI4

#ifdef MSC6
            printf( "%ld", (int32_t) g_oi.rarg1 );
#ifndef NDEBUG
            trace( "syscall integer: %ld\n", (int32_t) g_oi.rarg1 );
#endif /* NDEBUG */

#else /* MSC6 */
            printf( "%d", (int32_t) g_oi.rarg1 );
#ifndef NDEBUG
            trace( "syscall integer: %d\n", (int32_t) g_oi.rarg1 );
#endif /* NDEBUG */
#endif /* MSC6 */
#endif /* OI4 */

#ifdef OI8
            printf( "%lld", (int64_t) g_oi.rarg1 );
#ifndef NDEBUG
            trace( "syscall integer: %lld\n", (int64_t) g_oi.rarg1 );
#endif
#endif
            break;
        }
        default: { printf( "unhandled syscall!\n" ); break; }
    }
} /* OISyscall */

void OIHalt()
{
    g_halted = 1;
} /* OIHalt */

void usage()
{
    printf( "usage: oios [flags] <appname.oi>\n" );
    printf( "    OneImage Operating System.\n" );
    printf( "    flags:\n" );
    printf( "        -h      Show image headers then exit\n" );
#ifndef NDEBUG
    printf( "        -i      Enable instruction tracing if tracing is enabled\n" );
    printf( "        -p      Show performance information\n" );
    printf( "        -t      Enable tracing to oios.log\n" );
#endif
    exit( 1 );
} /* usage */

#ifdef OLDCPU
int main( argc, argv ) int argc; char * argv[];
#else
int cdecl main( int argc, char * argv[] )
#endif
{
    size_t result;
    char * input, * pc, * parg, c, ca;
    FILE * fp;
    int i;
    bool show_image_header, tracing, instruction_tracing, show_perf;
    uint32_t total_instructions;
    uint8_t image_width;
    struct OIHeader h;
    static char appname[ 80 ];

    total_instructions = 0;
    input = 0;
    tracing = false;
    instruction_tracing = false;
    show_image_header = false;
    show_perf = false;

    for ( i = 1; i < argc; i++ )
    {
        parg = argv[i];
        c = *parg;
    
        if ( ( 0 == input ) && ( '-' == c ) )
        {
            ca = (char) tolower( parg[1] );
            if ( 'h' == ca )
                show_image_header = true;
#ifndef NDEBUG
            else if ( 'i' == ca )
                instruction_tracing = true;
            else if ( 'p' == ca )
                show_perf = true;
            else if ( 't' == ca )
                tracing = true;
#endif
            else
                usage();
        }
        else if ( 0 == input )
            input = argv[ i ];
    }

    if ( 0 == input )
    {
        printf( "no input filename specified\n" );
        usage();
    }

#ifndef NDEBUG
    if ( tracing )
        enable_trace( "oios.log" );
#endif

    strcpy( appname, input );

    pc = strchr( appname, '.' );
    if ( !pc )
        strcat( appname, ".oi" );

#ifdef AZTECCPM
    fp = fopen( appname, "r" );
#else
    fp = fopen( appname, "rb" );
#endif
    if ( !fp )
    {
        printf( "can't open image file '%s'\n", appname );
        usage();
    }

    result = fread( &h, sizeof( h ), 1, fp );
    if ( 1 != result )
    {
        printf( "can't read image file header\n" );
        usage();
    }

    if ( show_image_header )
    {
        printf( "  signature:                %c%c\n", h.sig0, h.sig1 );
        printf( "  version:                  %u\n", h.version );
        printf( "  flags:                    %04xh\n", h.flags );
        printf( "  ram required:             %u\n", h.loRamRequired );
        printf( "  code size:                %u\n", h.cbCode );
        printf( "  initialized data size:    %u\n", h.cbInitializedData );
        printf( "  zero-filled data size:    %u\n", h.cbZeroFilledData );
        printf( "  stack size:               %u\n", h.cbStack );
        printf( "  initial PC:               %u\n", h.loInitialPC );
        exit( 0 );
    }

    if ( 'O' != h.sig0 || 'I' != h.sig1 )
    {
        printf( "image signature isn't the expected OI\n" );
        usage();
    }

    image_width = h.flags;
    if ( 0 == image_width )
        image_width = 2;
    else if ( 1 == image_width )
        image_width = 4;
    else if ( 2 == image_width )
        image_width = 8;
    else
    {
        printf( "image width in header is malformed\n" );
        usage();
    }

#ifdef OI2
    if ( 2 != image_width )
    {
        printf( "this version of oios only supports 2-byte image width binaries, and this one has %u\n", image_width );
        exit( 1 );
    }
#endif
#ifdef OI4
    if ( 4 != image_width )
    {
        printf( "this version of oios only supports 4-byte image width binaries, and this one has %u\n", image_width );
        exit( 1 );
    }
#endif
#ifdef OI8
    if ( 8 != image_width )
    {
        printf( "this version of oios only supports 8-byte image width binaries, and this one has %u\n", image_width );
        exit( 1 );
    }
#endif

    ram = ResetOI( (oi_t) h.loRamRequired, (oi_t) h.loInitialPC, image_width );
    if ( 0 == ram )
    {
        printf( "insufficient RAM for this application\n" );
        usage();
    }

    result = fread( ram, (uint16_t) h.cbCode + (uint16_t) h.cbInitializedData, 1, fp );
    if ( 1 != result )
    {
        printf( "can't read image file\n" );
        usage();
    }

    fclose( fp );

#ifndef NDEBUG
    TraceInstructionsOI( instruction_tracing );
#endif

    do
    {
        total_instructions += ExecuteOI();
    } while ( !g_halted );

#ifndef NDEBUG
    if ( show_perf )
        printf( "total instructions executed: %lu\n", total_instructions );

    if ( tracing )
        close_trace();
#endif

    return 0;
} /* main */

