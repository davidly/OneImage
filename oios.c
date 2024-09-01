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
uint32_t ram_size = 0;
uint8_t image_width;

#ifdef AZTECCPM
/* note that first two arguments are reversed */
#define memcpy( dest, src, length ) movmem( src, dest, (int) length )
#endif

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
            if ( 2 == image_width )
            {
                printf( "%d", (int16_t) g_oi.rarg1 );
#ifndef NDEBUG
                trace( "syscall integer: %d\n", (int16_t) g_oi.rarg1 );
#endif
            }
            else if ( 4 == image_width )
            {
#ifdef MSC6
                printf( "%ld", (int32_t) g_oi.rarg1 );
#ifndef NDEBUG
                trace( "syscall integer: %ld\n", (int32_t) g_oi.rarg1 );
#endif
#else /* MSC6 */
                printf( "%d", (int32_t) g_oi.rarg1 );
#ifndef NDEBUG
                trace( "syscall integer: %d\n", (int32_t) g_oi.rarg1 );
#endif
#endif /* MSC6 */
            }

#ifdef OI8
            else if ( 8 == image_width )
            {
                printf( "%lld", (int64_t) g_oi.rarg1 );
#ifndef NDEBUG
                trace( "syscall integer: %lld\n", (int64_t) g_oi.rarg1 );
#endif
            }
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

#ifdef OLDCPU
static size_t round_up( x, multiple ) size_t x; size_t multiple;
#else
static size_t round_up( size_t x, size_t multiple )
#endif
{
    size_t remainder;

    if ( (size_t) 0 == multiple )
        return x;

    remainder = x % multiple;
    if ( (size_t) 0 == remainder )
        return x;

    return x + multiple - remainder;
} /* round_up */

#ifdef OLDCPU
size_t size_args_env( appname, argc, argv, pchild_argc, first_child_arg )
    char * appname; int argc; char * argv[]; int *pchild_argc; int first_child_arg;
#else
size_t size_args_env( char * appname, int argc, char * argv[], int *pchild_argc, int first_child_arg )
#endif
{
    size_t len, head_len;
    int i;

    len = strlen( appname );
    head_len = (int) ( 6 * sizeof( oi_t ) + ( 1 + len ) );

    if ( -1 != first_child_arg )
    {
        *pchild_argc = 1 + ( argc - first_child_arg );
        for ( i = first_child_arg; i < argc; i++ )
            head_len += (int) ( sizeof( oi_t ) + ( 1 + strlen( argv[ i ] ) ) );
    }

    /* make sure the stack is image width aligned */
    head_len = round_up( head_len, sizeof( oi_t ) );
    return head_len;
} /* size_args_env */

#ifdef OLDCPU
void init_args_env( appname, argc, argv, child_argc, first_child_arg, head_len )
    char * appname; int argc; char * argv[]; int child_argc; int first_child_arg; size_t head_len;
#else
void init_args_env( char * appname, int argc, char * argv[], int child_argc, int first_child_arg, size_t head_len )
#endif
{
    int i;
    oi_t offset, x;

    offset = (oi_t) ( ram_size - head_len );
    x = (oi_t) child_argc;
    memcpy( &ram[ offset ], &x, sizeof( oi_t ) );                           /* argc */
    offset += sizeof( oi_t );
    x = offset + ( 2 * sizeof( oi_t ) );                                    /* get past argv and penv to argv array */
    memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                          /* argv */
    offset += sizeof( oi_t );
    x = offset + (oi_t) ( ( child_argc + 2 ) * sizeof( oi_t ) );
    memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                          /* penv */
    x += sizeof( oi_t );                                                    /* reserve 1 blank environment entry */
    offset += sizeof( oi_t );

    strcpy( (char *) & ram[ x ], appname );
    memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                          /* argv[0] */
    offset += sizeof( oi_t );
    x += (oi_t) ( 1 + strlen( appname ) );

    if ( -1 != first_child_arg )
    {
        for ( i = first_child_arg; i < argc; i++ )
        {
            strcpy( (char *) & ram[ x ], argv[ i ] );
            memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                  /* argv[1..n] */
            offset += sizeof( oi_t );
            x += (oi_t) ( 1 + strlen( argv[ i ] ) );
        }
    }

    x = 0;
    memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                          /* 0 terminator to argv array */
    offset += sizeof( oi_t );
    memcpy( & ram[ offset ], &x, sizeof( oi_t ) );                          /* 0 terminator to env array */
    offset += sizeof( oi_t );

#ifndef NDEBUG
    trace( "argument and environment information:\n" );
    trace_binary_data( & ram[ ram_size - head_len ], head_len, 2 );
#endif
} /* init_args_env */

static void usage()
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
    size_t result, head_len;
    char * input, * pc, * parg, c, ca;
    FILE * fp;
    int i, first_child_arg, child_argc;
    bool show_image_header, tracing, instruction_tracing, show_perf;
    uint32_t total_instructions, ram_requirement;
    struct OIHeader h;
    static char appname[ 80 ];

    total_instructions = 0;
    input = 0;
    tracing = false;
    instruction_tracing = false;
    show_image_header = false;
    show_perf = false;
    first_child_arg = -1;
    child_argc = 1;
    head_len = 0;

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
        else
        {
            first_child_arg = i;
            break;
        }
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
    if ( image_width > 4 )
    {
        printf( "this version of oios only supports 2- and 4-byte image width binaries, and this one has %u\n", image_width );
        exit( 1 );
    }
#endif

    head_len = size_args_env( appname, argc, argv, & child_argc, first_child_arg );
    ram_requirement = (uint32_t) ( h.loRamRequired + head_len );
    ram_size = RamInformationOI( ram_requirement, & ram, image_width );
    if ( 0 == ram )
    {
        printf( "insufficient RAM for this application. required %u, available %u\n", (int) ram_requirement, (int) ram_size );
        usage();
    }

    /* write the environment and argument info above where the top of stack will be */
    init_args_env( appname, argc, argv, child_argc, first_child_arg, head_len );

    ResetOI( (oi_t) h.loRamRequired, (oi_t) h.loInitialPC, (oi_t) ( ram_size - head_len ), image_width );

    result = fread( ram, (int) ( h.cbCode + h.cbInitializedData ), 1, fp );
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

