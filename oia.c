/*
    Assembler for OneImage
    Written by David Lee in August 2024.
    Note: there are extra, unnecessary casts to appease older compilers.
*/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#ifndef MSC6
#include <stdint.h>
#endif

#ifndef AZTECCPM
#include <string.h>
#endif

#include "oi.h"
#include "oios.h"

#ifdef MSC6
typedef uint32_t width_t;
typedef int32_t iwidth_t;
#else
typedef uint64_t width_t;
typedef int64_t iwidth_t;
#endif

#ifdef __GNUC__
#define stricmp strcasecmp
#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )
#endif

uint8_t g_image_width = 2;
uint8_t g_byte_len = 0;

#define true 1
#define false 0
#define MAX_LABELS 1000
#define MAX_LINE_LEN 200
#define MAX_TOKEN_LEN 80
#define MAX_TOKENS_PER_LINE 8

enum TokenTypes
{
    T_INVALID = 0, T_DATA, T_DATAEND, T_CODE, T_CODEEND,
    T_STRING, T_WORD, T_BYTE, T_IMAGE_T, T_ALIGN, T_DEFINE, T_LABEL,
    T_LD, T_LDB, T_LDINC, T_LDO, T_LDOB, T_LDOINC, T_LDOINCB, T_LDF, T_LDAE, T_LDI, T_LDIB,
    T_ST, T_STI, T_STB, T_STIB, T_STINC, T_STINCB, T_STO, T_STOB, T_STF, T_STWAE,
    T_J, T_JI, T_JREL, T_JRELB, T_SHL, T_SHLIMG, T_SHR, T_SHRIMG, T_MEMF, T_MEMFB, T_STADDB,
    T_ADD, T_SUB, T_MUL, T_DIV, T_OR, T_XOR, T_AND, T_CMP,
    T_INC, T_DEC, T_JMP, T_ADDST, T_SUBST, T_IDIVST, T_IMULST,
    T_CMPST, T_PUSH, T_POP, T_PUSHF, T_STST, T_ZERO, T_SYSCALL, T_MODDIV,
    T_RZERO, T_RPC, T_RSP, T_RFRAME, T_RARG1, T_RARG2, T_RRES, T_RTMP,
    T_GT, T_LT, T_EQ, T_NE, T_GE, T_LE, T_MOV, T_RET, T_RETZERO, T_RETNF, T_RETZERONF, T_INV,
    T_CSTF, T_MATHST, T_MATH, T_PLUS, T_IMGWID, T_ADDIMGW, T_SUBIMGW,
    T_SIGNEXB, T_SIGNEXW, T_SIGNEXDW,
    T_CALLNF, T_CALL
};

static const char * TokenSet[] =
{
    "INVALID", ".DATA", ".DATAEND", ".CODE", ".CODEEND",
    "STRING", "WORD", "BYTE", "IMAGE_T", "ALIGN", "DEFINE", "LABEL",
    "LD", "LDB", "LDINC", "LDO", "LDOB", "LDOINC", "LDOINCB", "LDF", "LDAE", "LDI", "LDIB",
    "ST", "STI", "STB", "STIB", "STINC", "STINCB", "STO", "STOB", "STF", "STWAE",
    "J", "JI", "JREL", "JRELB", "SHL", "SHLIMG", "SHR", "SHRIMG", "MEMF", "MEMFB", "STADDB",
    "ADD", "SUB", "MUL", "DIV", "OR", "XOR", "AND", "CMP",
    "INC", "DEC", "JMP", "ADDST", "SUBST", "IDIVST", "IMULST",
    "CMPST", "PUSH", "POP", "PUSHF", "STST", "ZERO", "SYSCALL", "MODDIV",
    "RZERO", "RPC", "RSP", "RFRAME", "RARG1", "RARG2", "RRES", "RTMP",
    "GT", "LT", "EQ", "NE", "GE", "LE", "MOV", "RET", "RETZERO", "RETNF", "RETZERONF", "INV",
    "CSTF", "MATHST", "MATH", "+", "IMGWID", "ADDIMGW", "SUBIMGW",
    "SIGNEXB", "SIGNEXW", "SIGNEXDW",
    "CALLNF", "CALL"
};

bool is_reg( size_t t ) { return ( t >= T_RZERO && t <= T_RTMP ); }
bool is_relation_token( size_t t ) { return ( t >= T_GT && t <= T_LE ); }
uint8_t reg_from_token( size_t t ) { return (uint8_t) ( t - T_RZERO ); }
uint8_t relation_from_token( size_t t ) { return (uint8_t) ( t - T_GT ); }
uint8_t math_from_token( size_t t ) { return (uint8_t) ( t - T_ADD ); }
bool is_data_token( size_t t ) { return ( t >= T_STRING && t <= T_WORD ); }
bool is_code_token( size_t t ) { return ( t >= T_LABEL && t <= T_CALL ); }
bool is_math_token( size_t t ) { return ( t >= T_ADD && t <= T_CMP ); }

struct LabelItem
{
    char * plabel;
    width_t datasize;
    width_t offset;
    bool initialized;
};

struct DefineItem
{
    char * pdefine;
    width_t value;
};

width_t g_cLabels = 0;
width_t g_labelCapacity = 0;
width_t g_cDefines = 0;
width_t g_defineCapacity = 0;
size_t line = 0;
struct LabelItem ** g_pLabels = 0;
struct DefineItem ** g_pDefines = 0;
static char original_line[ MAX_LINE_LEN ];
static char buf[ MAX_LINE_LEN ];
static char tokens[ MAX_TOKENS_PER_LINE ][ MAX_TOKEN_LEN ];
static uint8_t code[ 32767 ];
static width_t offsets[ 2000 ];

uint8_t compose_op( uint16_t f, uint16_t r, uint16_t w )
{
    assert( f <= 7 );
    assert( r <= 7 );
    assert( w <= 3 );
    return (uint8_t) ( ( f << 5 ) | ( r << 2 ) | w );
} /* compose_op */

void show_error( const char * p )
{
    printf( "error: %s on line %d: %s\n", p, (int) line, original_line );
    exit( 1 );
} /* show_error */

void print_space( FILE * fp, size_t len )
{
    size_t x;
    for ( x = 0; x < len; x++ )
        fprintf( fp, " " );
} /* print_space */

void * my_malloc( int cb )
{
    void * p;
    p = malloc( cb );
    if ( 0 == p )
        show_error( "can't allocate memory" );
    return p;
} /* my_malloc */

const char * render_width_t( width_t x )
{
    static char ac[ 40 ];
    ac[ 0 ] = 0;

    if ( 2 == g_image_width )
        sprintf( ac, "%04x", (unsigned short) x );
    else if ( 4 == g_image_width )
        sprintf( ac, "%08x", (unsigned int) x );
#ifndef MSC6
    else if ( 8 == g_image_width )
        sprintf( ac, "%016llx", (unsigned long long) x );
#endif
    return ac;
} /* render_width_t */

void show_labels()
{
    size_t i;
    printf( "  labels:\n" );
    for ( i = 0; i < (size_t) g_cLabels; i++ )
        printf( "    %s: %s\n", render_width_t( g_pLabels[ i ]->offset ), g_pLabels[ i ]->plabel );
} /* show_labels */

struct LabelItem * find_label( const char * p )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cLabels; i++ )
        if ( !stricmp( p, g_pLabels[ i ]->plabel ) )
            return g_pLabels[ i ];
    show_labels();
    printf( "missing label: '%s'\n", p );
    show_error( "can't find label" );
    return 0;
} /* find_label */

struct DefineItem * find_define( const char * p )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cDefines; i++ )
        if ( !stricmp( p, g_pDefines[ i ]->pdefine ) )
            return g_pDefines[ i ];
    return 0;
} /* find_define */

width_t get_define( const char * p )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cDefines; i++ )
        if ( !stricmp( p, g_pDefines[ i ]->pdefine ) )
            return g_pDefines[ i ]->value;
    show_error( "internal error: define can't be found" );
    return 0;
} /* get_define */

bool label_exists( const char * p )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cLabels; i++ )
        if ( !stricmp( p, g_pLabels[ i ]->plabel ) )
            return true;

    return false;
} /* label_exists */

bool define_exists( const char * p )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cDefines; i++ )
        if ( !stricmp( p, g_pDefines[ i ]->pdefine ) )
            return true;

    return false;
} /* define_exists */

const char * lookup_label( uint32_t offset )
{
    size_t i;
    for ( i = 0; i < (size_t) g_cLabels; i++ )
        if ( offset == g_pLabels[ i ]->offset )
            return g_pLabels[ i ]->plabel;

    return 0;
} /* lookup_label */

void add_label( const char * p, width_t datasize, int initialized, width_t offset )
{
    char * pdup;
    size_t len;
    struct LabelItem * pitem;
    struct LabelItem ** pitems;

    if ( label_exists( p ) )
        show_error( "duplicate label" );

    if ( define_exists( p ) )
        show_error( "label already declared as a define" );

    len = 1 + strlen( p );
    pdup = (char *) my_malloc( (int) len );
    memcpy( pdup, p, len );
    pitem = (struct LabelItem *) my_malloc( (int) sizeof( struct LabelItem ) );
    pitem->plabel = pdup;
    pitem->datasize = datasize;
    pitem->initialized = initialized;
    pitem->offset = offset;

    if ( (width_t) 0 == g_labelCapacity )
    {
        g_labelCapacity = 4;
        g_pLabels = (struct LabelItem **) my_malloc( (int) g_labelCapacity * sizeof( void * ) );
    }

    if ( g_cLabels == g_labelCapacity )
    {
        pitems = (struct LabelItem **) my_malloc( (int) g_labelCapacity * 2 * sizeof( void * ) );
        memcpy( pitems, g_pLabels, (size_t) g_labelCapacity * sizeof( void * ) );
        g_labelCapacity *= 2;
        free( g_pLabels );
        g_pLabels = pitems;
    }

    g_pLabels[ g_cLabels++ ] = pitem;
} /* add_label */

void add_define( const char * p, width_t value )
{
    char * pdup;
    size_t len;
    struct DefineItem * pitem;
    struct DefineItem ** pitems;

    if ( define_exists( p ) )
        show_error( "duplicate define" );

    if ( label_exists( p ) )
        show_error( "define already declared as a label" );

    len = 1 + strlen( p );
    pdup = (char *) my_malloc( (int) len );
    memcpy( pdup, p, len );
    pitem = (struct DefineItem *) my_malloc( (int) sizeof( struct DefineItem ) );
    pitem->pdefine = pdup;
    pitem->value = value;

    if ( (width_t) 0 == g_defineCapacity )
    {
        g_defineCapacity = 4;
        g_pDefines = (struct DefineItem **) my_malloc( (int) g_defineCapacity * sizeof( void * ) );
    }

    if ( g_cDefines == g_defineCapacity )
    {
        pitems = (struct DefineItem**) my_malloc( (int) g_defineCapacity * 2 * sizeof( void * ) );
        memcpy( pitems, g_pDefines, (size_t) g_defineCapacity * sizeof( void * ) );
        g_defineCapacity *= 2;
        free( g_pDefines );
        g_pDefines = pitems;
    }

    g_pDefines[ g_cDefines++ ] = pitem;
} /* add_define */

void usage()
{
    printf( "usage: oia [flags] <source.s>\n" );
    printf( "  OneImage assembler. produces <source>.oi, which can be run in oios.\n" );
    printf( "  flags:\n" );
    printf( "      -i          show information about the generated image\n" );
    printf( "      -l          create listing file <source>.lst\n" );
    printf( "      -t          show verbose tracing as assembly happens\n" );
    printf( "      -w:X        image width: 2, 4, or 8 bytes. Default is 2.\n" );
    exit( 1 );
} /* usage */

bool is_blank( char c ) { return ( ( ',' == c ) || ( ' ' == c ) || ( 9 == c ) || ( 13 == c ) || ( 10 == c ) || '[' == c || ']' == c ); }

bool is_token( char c ) { return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                                  ( ':' == c ) || ( '.' == c ) || ( '_' == c ) || ( '-' == c ) ||
                                  ( c >= '0' && c <= '9' ) || ( '+' == c ) ); }

bool is_digit( char c ) { return ( c >= '0' && c <= '9' ); }

bool is_number( const char * p )
{
    if ( !*p )
        return false;

    if ( '-' == *p )
        p++;

    if ( !*p )
        return false;

    while ( *p )
    {
        if ( !is_digit( *p ) )
            return false;
        p++;
    }

    return true;
} /* is_number */

void rm_white( char * p )
{
    size_t len;
    char *porig = p;

    while ( is_blank( *p ) )
        p++;

    len = strlen( p );
    while ( len && is_blank( p[ len - 1] ) )
        len--;

    p[ len ] = 0;
    memmove( porig, p, len + 1 );
} /* rm_white */

void unescape( char * p )
{
    size_t len;

    while ( *p )
    {
        if ( '\\' == *p )
        {
            if ( 'n' == *( p + 1 ) )
            {
                *p++ = 10;
                len = strlen( p + 1 );
                memmove( p, p + 1, len + 1 );
            }
            else
                show_error( "unrecognized escape sequence" );
        }
        p++;
    }
} /* unescape */

int tokenize( char * p )
{
    int i, c;
    char * pnext;
    i = 0;
    c = 0;

    while ( *p && c < MAX_TOKENS_PER_LINE )
    {
        if ( '\"' == *p )
        {
            pnext = strrchr( p + 1, '\"' );
            if ( !pnext )
                show_error( "string has no termination" );

            memcpy( tokens[ c ], p + 1, pnext - p - 1 );
            tokens[ c ][ pnext - p - 1 ] = 0;
            unescape( tokens[ c ] );
            c++;
            p = pnext + 1;
            continue;
        }

        while ( is_token( *p ) )
            tokens[ c ][ i++ ] = *p++;
        while ( is_blank( *p ) )
            p++;

        tokens[ c ][ i ] = 0;
        c++;
        i = 0;
    }
    return c;
} /* tokenize */

int find_token( char * p )
{
    int i;
    for ( i = 0; i < _countof( TokenSet ); i++ )
    {
        if ( !stricmp( p, TokenSet[ i ] ) )
            return i;
    }

    return T_INVALID;
} /* find_token */

width_t number_or_define( const char * p )
{
    if ( is_number( p ) )
#ifdef MSC6
        return atoi( p );
#else
        return strtoll( p, 0, 10 );
#endif
    else if ( find_define( p ) )
        return get_define( p );

    show_error( "a number or define is expected" );
    return 0;
} /* number_or_define */

static char acfile[ 80 ];
static char aclistfile[ 80 ];

#ifdef OLDCPU
width_t round_up( x, multiple ) width_t x; width_t multiple;
#else
width_t round_up( width_t x, width_t multiple )
#endif
{
    width_t remainder;

    if ( (width_t) 0 == multiple )
        return x;

    remainder = x % multiple;
    if ( (width_t) 0 == remainder )
        return x;

    return x + multiple - remainder;
} /* round_up */

#ifdef OLDCPU
void check_if_in_i16_range( val ) iwidth_t val;
#else
void check_if_in_i16_range( iwidth_t val )
#endif
{
    if ( val < -32768 || val > 32767 )
        show_error( "value must be in the range of -32768..32767" );
} /* check_if_in_i16_range */

#ifdef OLDCPU
void width_zero_check( offset ) width_t offset;
#else
void width_zero_check( width_t offset )
#endif
{
    uint8_t x;
    for ( x = 0; x < g_image_width; x++ )
    {
        if ( 0 != code[ offset + x ] )
            show_error( "internal error in second pass: offset isn't zero in width check" );
    }
} /* width_zero_check */

#ifdef OLDCPU
void word_zero_check( offset ) width_t offset;
#else
void word_zero_check( width_t offset )
#endif
{
    uint8_t x;
    for ( x = 0; x < sizeof( uint16_t ); x++ )
    {
        if ( 0 != code[ offset + x ] )
            show_error( "internal error in second pass: offset isn't zero in word check" );
    }
} /* word_zero_check */

#ifdef OLDCPU
void initialie_image_value( pcode, value ) width_t * pcode; width_t value;
#else
void initialize_image_value( width_t * pcode, width_t value )
#endif
{
    memcpy( code + ( * pcode ), &value, g_image_width );
    *pcode += g_image_width;
} /* initialize_image_value */

#ifdef OLDCPU
void initialie_word_value( pcode, value ) width_t * pcode; width_t value;
#else
void initialize_word_value( width_t * pcode, width_t value )
#endif
{
    * ( uint16_t *) ( code + ( *pcode ) ) = (uint16_t) value;
    *pcode += sizeof( uint16_t );
} /* initialize_word_value */

#ifdef OLDCPU
int main( argc, argv ) int argc; char * argv[];
#else
int cdecl main( int argc, char * argv[] )
#endif
{
    FILE * fp;
    char * p, * input;
    const char * pc;
    size_t l, t, t1, t2, t3, t4;
    uint16_t u16val;
    int16_t i16val, result, offset;
    width_t len, x, size, val, j;
    iwidth_t diff, ival, arg, alignment, num;
    uint8_t reg, tmp, width;
    int i, data_mode, code_mode, token_count;
    width_t initialized_data_so_far, total_zeroed_data, code_so_far, total_initialized_data, total_code;
    width_t initialized_data_offset, zeroed_data_offset;
    bool is_register, show_image_info, show_verbose_tracing, create_listing;
    struct LabelItem * plabel;
    struct DefineItem * pdefine;
    struct OIHeader h;

    create_listing = false;
    show_image_info = false;
    show_verbose_tracing = false;
    input = 0;
    data_mode = 0;
    code_mode = 0;
    initialized_data_so_far = 0;
    total_zeroed_data = 0;
    total_initialized_data = 0;
    code_so_far = 0;
    total_code = 0;
    initialized_data_offset = 0;
    zeroed_data_offset = 0;
    g_image_width = 2;

    if ( T_CALL != ( _countof( TokenSet ) - 1 ) )
        show_error( "token parallel arrays are broken" );

    for ( i = 1; i < argc; i++ )
    {
        char *parg = argv[i];
        char c = *parg;
    
        if ( ( 0 == input ) && ( '-' == c ) )
        {
            char ca = (char) tolower( parg[1] );
    
            if ( 'i' == ca )
                show_image_info = true;
            else if ( 'l' == ca )
                create_listing = true;
            else if ( 't' == ca )
                show_verbose_tracing= true;
            else if ( 'w' == ca )
            {
                if ( ':' != parg[2] )
                    usage();
                g_image_width = (uint8_t) atoi( parg + 3 );
                if ( 2 != g_image_width && 4 != g_image_width && 8 != g_image_width )
                    usage();
            }
            else
                usage();
        }
        else if ( 0 == input )
            input = argv[ i ];
    }

    if ( 2 == g_image_width )
        g_byte_len = 1;
    else if ( 4 == g_image_width )
        g_byte_len = 2;
    else if ( 8 == g_image_width )
        g_byte_len = 3;

    if ( 0 == input )
    {
        printf( "no input filename specified\n" );
        usage();
    }

    strcpy( acfile, input);
    p = strstr( acfile, ".s" );
    if ( !p )
        strcat( acfile, ".s" );

    fp = fopen( acfile, "r" );
    if ( !fp )
    {
        printf( "can't open input file\n" );
        usage();
    }

    /* no native syscall handler */
    initialize_image_value( & code_so_far, 0 );

    line = 0;
    while ( fgets( buf, sizeof( buf ), fp ) )
    {
        line++;
        offsets[ line ] = code_so_far;
        strcpy( original_line, buf );
        p = strchr( (char *) buf, ';' );
        if ( p )
            *p = 0;
        rm_white( buf );

        len = (width_t) strlen( buf );
        if ( (width_t) 0 == len )
            continue;

        token_count = tokenize( buf );

        if ( show_verbose_tracing )
        {
            printf( "line %d has token count: %d -- %s\n", (int) line, token_count, buf );
            printf( "  code_so_far: %s\n", render_width_t( code_so_far ) );
            for ( t = 0; t < (size_t) token_count; t++ )
                printf( "  token %d: '%s' has type %d == %s\n", (int) t, tokens[ t ],
                        find_token( tokens[ t ] ), TokenSet[ find_token( tokens[ t ] ) ] );
        }

        len = (width_t) strlen( buf );
        if ( ':' == buf[ len - 1 ] )
        {
            buf[ len - 1 ] = 0;
            add_label( buf, 0, false, code_so_far );
            continue;
        }

        t = find_token( tokens[ 0 ] );
        //printf( "  token at line %d: %d == %s\n", (int) line, (int) t, TokenSet[ t ] );

        if ( is_code_token( t ) && ( 1 != code_mode ) )
            show_error( "code must be in a .code block" );

        if ( is_data_token( t ) && ( 1 != data_mode ) )
            show_error( "data must be in a .data block" );

        switch( t )
        {
            case T_INVALID: { show_error( "invalid token; is it a label without a trailing ':'?" ); }
            case T_DATA:
            {
                if ( 0 != code_mode )
                    show_error( "data section must come before code" );
                if ( 0 != data_mode )
                    show_error( "only one data section is allowed" );
                if ( 1 != token_count )
                    show_error( ".data has unexpected text" );
                data_mode++;
                break;
            }
            case T_DATAEND:
            {
                if ( 1 != data_mode )
                    show_error( ".dataend while not in a data block is not allowed" );
                if ( 1 != token_count )
                    show_error( ".dataend has unexpected text" );
                data_mode++;
                break;
            }
            case T_CODE:
            {
                if ( 0 != code_mode )
                    show_error( "only one code mode is allowed" );
                if ( 1 != token_count )
                    show_error( ".code has unexpected text" );
                code_mode++;
                break;
            }
            case T_CODEEND:
            {
                if ( 1 != code_mode )
                    show_error( ".codeend while not in a data block is not allowed" );
                if ( 1 != token_count )
                    show_error( ".codeend has unexpected text" );
                code_mode++;
                break;
            }
            case T_ALIGN:
            {
                if ( 1 != data_mode && 1 != code_mode )
                    show_error( "align only expected in a .data or .code section" );
                if ( token_count > 2 )
                    show_error( "align requires zero or one arguments" );
                if ( 2 == token_count )
                {
                    arg = number_or_define( tokens[ 1 ] );
                    if ( (uint32_t) 2 != arg && (uint32_t) 4 != arg && (uint32_t) 8 != arg )
                        show_error( "align requires an argument of 2, 4, or 8 (bytes implied)" );
                    alignment = arg;
                }
                else
                    alignment = g_image_width;

                if ( 1 == data_mode )
                {
                    /* align both because we don't know what's next */
                    total_zeroed_data = round_up( total_zeroed_data, alignment );
                    initialized_data_so_far = round_up( initialized_data_so_far, alignment );
                }
                else
                    code_so_far = round_up( code_so_far, alignment );
                break;
            }
            case T_DEFINE:
            {
                if ( 3 != token_count )
                    show_error( "define statements must have two arguments" );

                if ( !is_number( tokens[ 2 ] ) )
                    show_error( "second argument must be a number" );

                add_define( tokens[ 1 ], (uint16_t) atoi( tokens[ 2 ] ) );
                break;
            }
            case T_BYTE:
            {
                if ( 2 != token_count && 3 != token_count )
                    show_error( "word data has a label and optional array size" );

                t1 = find_token( tokens[ 1 ] );
                if ( T_INVALID != t1 )
                    show_error( "word data has a label and optional array size" );

                p = strchr( buf, '[' );

                if ( p )
                {
                    if ( 3 != token_count )
                        show_error( "square bracket has no value" );
                    if ( is_number( tokens[ 2 ] ) )
                        size = (uint16_t) atoi( tokens[ 2 ] );
                    else
                    {
                         pdefine = find_define( tokens[ 2 ] );
                         if ( pdefine )
                            size = pdefine->value;
                        else
                            show_error( "data size must be a number or define" );
                    }
                }
                else
                    size = 1;

                if ( (uint32_t) 0 == size )
                    show_error( "word data has a label and optional non-zero array size" );

                add_label( tokens[ 1 ], size, false, 0 );
                total_zeroed_data += size;
                break;
            }
            case T_WORD:
            {
                if ( 2 != token_count && 3 != token_count )
                    show_error( "word data has a label and optional array size" );

                t1 = find_token( tokens[ 1 ] );
                if ( T_INVALID != t1 )
                    show_error( "word data has a label and optional array size" );

                p = strchr( buf, '[' );
                if ( p )
                {
                    if ( 3 != token_count )
                        show_error( "square bracket has no value" );
                    if ( is_number( tokens[ 2 ] ) )
                        size = 2 * (uint16_t) atoi( tokens[ 2 ] );
                    else
                    {
                         pdefine = find_define( tokens[ 2 ] );
                         if ( pdefine )
                            size = 2 * pdefine->value;
                        else
                            show_error( "data size must be a number or define" );
                    }
                }
                else
                    size = 2;

                if ( (width_t) 0 == size )
                    show_error( "word data has a label and optional non-zero array size" );

                add_label( tokens[ 1 ], size, false, 0 );
                total_zeroed_data += size;
                break;
            }
            case T_IMAGE_T:
            {
                if ( 2 != token_count && 3 != token_count )
                    show_error( "image_t data requires a label and optional array size" );

                t1 = find_token( tokens[ 1 ] );
                if ( T_INVALID != t1 )
                    show_error( "image_t data requires a label and optional array size" );

                p = strchr( buf, '[' );
                if ( p )
                {
                    if ( 3 != token_count )
                        show_error( "square bracket has no value" );
                    if ( is_number( tokens[ 2 ] ) )
                        size = g_image_width * (width_t) atoi( tokens[ 2 ] );
                    else
                    {
                         pdefine = find_define( tokens[ 2 ] );
                         if ( pdefine )
                            size = g_image_width * pdefine->value;
                        else
                            show_error( "data size must be a number or define" );
                    }
                }
                else
                    size = g_image_width;

                if ( (width_t) 0 == size )
                    show_error( "word data has a label and optional non-zero array size" );

                add_label( tokens[ 1 ], size, false, 0 );
                total_zeroed_data += size;
                break;
            }
            case T_STRING:
            {
                if ( 3 != token_count )
                    show_error( "string data has two arguments: label and value" );

                t1 = find_token( tokens[ 1 ] );
                if ( T_INVALID != t1 )
                    show_error( "string data has two arguments: label and value" );

                size = (uint16_t) ( 1 + strlen( tokens[ 2 ] ) );
                add_label( tokens[ 1 ], size, true, 0 );
                initialized_data_so_far += size;
                break;
            }
            case T_IMGWID:
            {
                if ( 1 != token_count )
                    show_error( "imgwid takes no arguments" );
                code[ code_so_far++ ] = 0x84;
                break;
            }
            case T_ADDIMGW:
            case T_SUBIMGW:
            {
                if ( 2 != token_count )
                    show_error( "addimgw and subimgw take one register argument: addimgw reg\n" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "addimgw and subimgw take one register argument: addimgw reg\n" );

                code[ code_so_far++ ] = compose_op( 4, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 3, 0, ( T_ADDIMGW == t ) ? 0 : 1 );
                break;
            }
            case T_STST:
            {
                if ( 2 != token_count )
                    show_error( "stst takes one register argument: stst [reg] -- the pop() is implied\n" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "stst takes one register argument: stst [reg] -- the pop() is implied\n" );

                code[ code_so_far++ ] = compose_op( 4, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 2, 0, 1 );
                break;
            }
            case T_SIGNEXB:
            case T_SIGNEXW:
            case T_SIGNEXDW:
            {
                if ( 2 != token_count )
                    show_error( "signex takes one register argument: signex reg\n" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "signex takes one register argument: signex reg\n" );

                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 4, 0, ( T_SIGNEXB == t ) ? 0 : ( T_SIGNEXW == t ) ? 1 : 2 );
                break;
            }
            case T_PUSHF:
            {
                if ( 2 != token_count )
                    show_error( "pushf requires 1 argument: an integer >= -4 and <= 3. e.g. pushf -2\n" );

                offset = (int16_t) number_or_define( tokens[ 1 ] );

                if ( offset < -4 || offset > 3 )
                    show_error( "pushf requires 1 arguments: an integer >= -4 and <= 3. e.g. pushf -2\n" );

                code[ code_so_far++ ] = compose_op( 4, 0, 1 );
                code[ code_so_far++ ] = compose_op( 1, offset, 1 );
                break;
            }
            case T_LDF:
            {
                if ( 3 != token_count )
                    show_error( "ldf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) || !is_number( tokens[ 2 ] ) )
                    show_error( "ldf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );

                offset = (int16_t) atoi( tokens[ 2 ] );
                if ( offset < -4 || offset > 3 )
                    show_error( "ldf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );

                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 0, offset, 1 );
                break;
            }
            case T_STF:
            {
                if ( 3 != token_count )
                    show_error( "stf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) || !is_number( tokens[ 2 ] ) )
                    show_error( "stf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );

                offset = (int16_t) atoi( tokens[ 2 ] );
                if ( offset < -4 || offset > 3 )
                    show_error( "stf requires 2 arguments, a register and integer >= -4 and <= 3. e.g. ldf rres, -2\n" );

                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 1, offset, 1 );
                break;
            }
            case T_SYSCALL:
            {
                if ( 2 != token_count )
                    show_error( "syscall takes two arguments" );

                u16val = (uint16_t) number_or_define( tokens[ 1 ] );
                code[ code_so_far++ ] = compose_op( 4, ( u16val >> 3 ) & 7, 1 );
                code[ code_so_far++ ] = compose_op( 0, ( u16val & 7 ), 0 );
                break;
            }
            case T_IMULST:
            {
                if ( 1 != token_count )
                    show_error( "imulst takes no arguments" );
                code[ code_so_far++ ] = compose_op( 1, 0, 0 );
                break;
            }
            case T_IDIVST:
            {
                if ( 1 != token_count )
                    show_error( "idivst takes no arguments" );
                code[ code_so_far++ ] = 0xa8;
                break;
            }
            case T_ADDST:
            {
                if ( 1 != token_count )
                    show_error( "iaddst takes no arguments" );
                code[ code_so_far++ ] = compose_op( 5, 0, 0 );
                break;
            }
            case T_SUBST:
            {
                if ( 1 != token_count )
                    show_error( "isubst takes no arguments" );
                code[ code_so_far++ ] = compose_op( 4, 0, 0 );
                break;
            }
            case T_MEMF:
            {
                if ( 1 != token_count )
                    show_error( "memf takes no arguments" );
                code[ code_so_far++ ] = compose_op( 3, 0, 1 );
                code[ code_so_far++ ] = compose_op( 5, 0, g_byte_len );
                break;
            }
            case T_MEMFB:
            {
                if ( 1 != token_count )
                    show_error( "memfb takes no arguments" );
                code[ code_so_far++ ] = compose_op( 3, 0, 1 );
                code[ code_so_far++ ] = compose_op( 5, 0, 0 );
                break;
            }
            case T_STADDB:
            {
                if ( 1 != token_count )
                    show_error( "memfb takes no arguments" );
                code[ code_so_far++ ] = compose_op( 3, 0, 1 );
                code[ code_so_far++ ] = compose_op( 6, 0, 0 );
                break;
            }
            case T_RETZERO:
            {
                code[ code_so_far++ ] = compose_op( 0, 2, 0 );
                break;
            }
            case T_RETNF:
            {
                code[ code_so_far++ ] = 0x68;
                break;
            }
            case T_RETZERONF:
            {
                code[ code_so_far++ ] = 0x48;
                break;
            }
            case T_RET:
            {
                if ( token_count > 2 )
                    show_error( "ret takes 0 or 1 arguments" );

                if ( 1 == token_count )
                    code[ code_so_far++ ] = compose_op( 6, 0, 0 );
                else
                {
                    num = number_or_define( tokens[ 1 ] );
                    if ( num < 1 || num > 8 )
                        show_error( "ret <constant> must be 1..8" );

                    code[ code_so_far++ ] = compose_op( 3, 0, 1 );
                    code[ code_so_far++ ] = compose_op( 2, (uint16_t) ( num - 1 ), 0 );
                }
                break;
            }
            case T_LDAE:
            {
                /* ldae (implied rres) address[ register ].  address plus ( register * image width  ) */

                if ( 3 != token_count )
                    show_error( "ldae takes two arguments" );

                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( T_INVALID != t1 )
                    show_error( "ldae first argument must be a label" );
                if ( !is_reg( t2 ) )
                    show_error( "ldae second argument must be a register" );

                reg = reg_from_token( t2 );

                code[ code_so_far++ ] = compose_op( 6, reg, 2 );
                initialize_image_value( & code_so_far, 0 );
                break;
            }
            case T_JMP:
            {
                // forms of jmp:
                //     jmp register (address 0 implied)
                //     jmp address (rzero implied)
                //     jmp address, register

                t1 = find_token( tokens[ 1 ] );
                if ( token_count > 3 )
                    show_error( "jmp has too many arguments" );

                val = 0;
                reg = 0; // rzero
                t2 = 0;

                if ( is_reg( t1 ) && ( 2 != token_count ) )
                    show_error( "jmp register only allows one argument" );
                else if ( T_INVALID != t1 )
                    show_error( "jmp label not found as second argument" );
                else if ( 3 == token_count )
                {
                    t2 = find_token( tokens[ 2 ] );
                    if ( !is_reg( t2 ) )
                        show_error( "jmp address register .. isn't a register" );
                }

                if ( is_reg( t1 ) )
                    reg = reg_from_token( t1 );
                else if ( 3 == token_count )
                    reg = reg_from_token( t2 );

                code[ code_so_far++ ] = compose_op( 3, reg, 2 );
                initialize_image_value( & code_so_far, 0 );
                break;
            }
            case T_CALL:
            {
                // forms of call:
                //     call register
                //     call address
                //     call address, register. Address is for a function pointer table. Register * 2 is added and the function is taken from the array

                t1 = find_token( tokens[ 1 ] );
                if ( token_count > 3 )
                    show_error( "call has too many arguments" );

                if ( 3 == token_count )
                    t2 = find_token( tokens[ 2 ] );
                else
                    t2 = 0; /* make the compiler shut up */

                if ( is_reg( t1 ) && ( 2 != token_count ) )
                    show_error( "call register only allows one argument" );
                else if ( T_INVALID != t1 )
                    show_error( "call label not found as second argument" );
                else if ( 3 == token_count && !is_reg( t2 ) )
                    show_error( "call address register .. isn't a register" );

                val = 0;
                reg = 0; // rzero

                if ( is_reg( t1 ) )
                    reg = reg_from_token( t1 );
                else if ( 3 == token_count )
                    reg = reg_from_token( t2 );

                if ( 3 == token_count )
                {
                    // 4-byte instruction

                    code[ code_so_far++ ] = compose_op( 3, reg, 3 );
                    code[ code_so_far++ ] = 0;
                    initialize_word_value( & code_so_far, 0 );
                }
                else
                {
                    code[ code_so_far++ ] = compose_op( 7, reg, 2 );
                    initialize_image_value( & code_so_far, 0 );
                }
                break;
            }
            case T_CALLNF:
            {
                // forms of callnf:
                //     callnf register
                //     callnf address
                //     callnf address[ register ]. Address is for a function pointer table. Register * 2 is added and the function is taken indirect from the array
                //     callnf address, register. register is added to address; no indirection or function table

                t1 = find_token( tokens[ 1 ] );
                if ( token_count > 3 )
                    show_error( "callnf has too many arguments" );

                if ( 3 == token_count )
                    t2 = find_token( tokens[ 2 ] );
                else
                    t2 = 0; /* make the compiler shut up */

                if ( is_reg( t1 ) && ( 2 != token_count ) )
                    show_error( "call register only allows one argument" );
                else if ( T_INVALID != t1 )
                    show_error( "call label not found as second argument" );
                else if ( 3 == token_count && !is_reg( t2 ) )
                    show_error( "call address register .. isn't a register" );

                val = 0;
                reg = 0; // rzero

                if ( is_reg( t1 ) )
                    reg = reg_from_token( t1 );
                else if ( 3 == token_count )
                    reg = reg_from_token( t2 );

                // 4-byte instruction

                code[ code_so_far++ ] = compose_op( 3, reg, 3 );
                code[ code_so_far++ ] = compose_op( strchr( buf, '[' ) ? 1 : 2, 0, 0 );
                initialize_word_value( & code_so_far, 0 );
                break;
            }
            case T_INC:
            {
                // forms of inc:
                //     inc register
                //     inc [register]
                //     inc [address]     (rzero implied)
                //     inc [address, register]

                is_register = ( 0 == strchr( original_line, '[' ) );
                t1 = find_token( tokens[ 1 ] );

                if ( is_register && 2 != token_count && !is_reg( t1 ) )
                    show_error( "inc direct requires a register\n" );

                if ( is_register )
                {
                    reg = reg_from_token( t1 );
                    if ( 0 == reg || 2 == reg )
                        show_error( "inc of rsp and rzero are invalid" );
                    code[ code_so_far++ ] = compose_op( 0, reg, 0 );
                }
                else
                {
                    if ( 3 == token_count )
                    {
                        t2 = find_token( tokens[ 2 ] );
                        reg = reg_from_token( t2 );
                    }
                    else if ( is_reg( t1 ) )
                        reg = reg_from_token( t1 );
                    else
                        reg = 0; // rzero

                    code[ code_so_far++ ] = compose_op( 4, reg, 2 );
                    initialize_image_value( & code_so_far, 0 );
                }
                break;
            }
            case T_DEC:
            {
                // forms of dec:
                //     dec register
                //     dec [register]
                //     dec [address]     (rzero implied)
                //     dec [address, register]

                is_register = ( 0 == strchr( original_line, '[' ) );
                t1 = find_token( tokens[ 1 ] );

                if ( is_register && 2 != token_count && !is_reg( t1 ) )
                    show_error( "dec direct requires a register\n" );

                if ( is_register )
                {
                    reg = reg_from_token( t1 );
                    if ( 0 == reg || 2 == reg )
                        show_error( "dec of rsp and rzero are invalid" );
                    code[ code_so_far++ ] = compose_op( 1, reg, 0 );
                }
                else
                {
                    if ( 3 == token_count )
                    {
                        t2 = find_token( tokens[ 2 ] );
                        reg = reg_from_token( t2 );
                    }
                    else if ( is_reg( t1 ) )
                        reg = reg_from_token( t1 );
                    else
                        reg = 0; // rzero

                    code[ code_so_far++ ] = compose_op( 5, reg, 2 );
                    initialize_image_value( & code_so_far, 0 );
                }
                break;
            }
            case T_ZERO:
            {
                t1 = find_token( tokens[ 1 ] );
                if ( ( 2 != token_count ) || ( ( 2 == token_count ) && ! is_reg( t1 ) ) )
                    show_error( "push takes a register argument" );

                reg = reg_from_token( t1 );
                code[ code_so_far++ ] = compose_op( 4, reg, 0 );
                break;
            }
            case T_PUSH:
            {
                t1 = find_token( tokens[ 1 ] );
                if ( ( 2 != token_count ) || ( ( 2 == token_count ) && ! is_reg( t1 ) ) )
                    show_error( "push takes a register argument" );

                reg = reg_from_token( t1 );
                code[ code_so_far++ ] = compose_op( 2, reg, 0 );
                break;
            }
            case T_POP:
            {
                t1 = find_token( tokens[ 1 ] );
                if ( ( 2 != token_count ) || ( ( 2 == token_count ) && ! is_reg( t1 ) ) )
                    show_error( "pop takes a register argument" );

                reg = reg_from_token( t1 );
                code[ code_so_far++ ] = compose_op( 3, reg, 0 );
                break;
            }
            case T_SHL:
            {
                if ( 2 != token_count )
                    show_error( "shl takes one argument - a register" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );

                code[ code_so_far++ ] = compose_op( 5, reg_from_token( t1 ), 0 );
                break;
            }
            case T_SHLIMG:
            {
                if ( 1 != token_count )
                    show_error( "shlimg takes no arguments" );

                code[ code_so_far++ ] = 0x28;
                break;
            }
            case T_SHR:
            {
                if ( 2 != token_count )
                    show_error( "shr takes one argument - a register" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );

                code[ code_so_far++ ] = compose_op( 6, reg_from_token( t1 ), 0 );
                break;
            }
            case T_SHRIMG:
            {
                if ( 1 != token_count )
                    show_error( "shrimg takes no arguments" );

                code[ code_so_far++ ] = 0x88;
                break;
            }
            case T_ADD:
            {
                if ( 3 != token_count )
                    show_error( "add takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 0 );
                break;
            }
            case T_DIV:
            {
                if ( 3 != token_count )
                    show_error( "div takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t2 ), 0 );
                break;
            }
            case T_MUL:
            {
                if ( 3 != token_count )
                    show_error( "mul takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 2, reg_from_token( t2 ), 0 );
                break;
            }
            case T_MODDIV:
            {
                if ( 3 != token_count )
                    show_error( "moddiv takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 7, reg_from_token( t2 ), 0 );
                break;
            }
            case T_STI:
            case T_STIB:
            {
                // st [address], CONSTANT -32..31

                if ( 3 != token_count )
                    show_error( "sti takes two arguments: an addres to write to and a constant -32..31" );

                val = 0;
                if ( is_number( tokens[ 1 ] ) || find_define( tokens[ 1 ] ) )
                    val = (uint16_t) number_or_define( tokens[ 1 ] );
                check_if_in_i16_range( val );

                i16val = (int16_t) number_or_define( tokens[ 2 ] );
                if ( i16val < -32 || i16val > 31 )
                    show_error( "sti immediate integer values must be in the range -32..31" );

                // r0 has high 3 bits and r1 has low 3 bits of CONSTANT

                code[ code_so_far++ ] = compose_op( 6, ( ( i16val >> 3 ) & 7 ), 3 );
                code[ code_so_far++ ] = compose_op( 1, ( i16val & 7 ), ( T_STIB == t ) ? 0 : g_byte_len );
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_ST:
            {
                // forms of st:
                //    st [rdst], rsrc
                //    st [CONSTANT], rsrc
                if ( 3 != token_count )
                    show_error( "st takes two arguments" );
                t1 = find_token( tokens[ 1 ] );

                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                if ( is_reg( t1 ) ) // st [rdst], rsrc
                {
                    code[ code_so_far++ ] = compose_op( 5, reg_from_token( t1 ), 1 );
                    code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), g_byte_len );
                }
                else // st [address], register
                {
                    if ( T_INVALID != t1 )
                        show_error( "label expected as first argument" );
    
                    code[ code_so_far++ ] = compose_op( 2, reg_from_token( t2 ), 2 );
                    initialize_image_value( & code_so_far, 0 );
                }
                break;
            }
            case T_LDOINC:
            case T_LDOINCB:
            case T_LDO:
            case T_LDOB:
            {
                if ( 4 != token_count )
                    show_error( "ldob 3 values: ldob rdst, address[ roffset]" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );

                if ( !is_reg( t1 ) || !is_reg( t3 ) )
                    show_error( "first and third arguments must be registers" );

                if ( !is_number( tokens[ 2 ] ) && !find_define( tokens[ 2 ] ) && ( T_INVALID != t2 ) ) 
                    show_error( "second argument must be an address" );

                val = 0;
                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                    val = number_or_define( tokens[ 2 ] );
                check_if_in_i16_range( val );

                tmp = (uint8_t) ( ( T_LDOINC == t || T_LDOINCB == t ) ? 1 : 0 );
                width = (uint8_t) ( ( T_LDOB == t || T_LDOINCB == t ) ? 0 : 1 );

                code[ code_so_far++ ] = compose_op( 5, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( tmp, reg_from_token( t3 ), width );
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_STOB:
            {
                if ( 4 != token_count )
                    show_error( "stob 3 values: stob address[ roffset ], rsrc" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );

                if ( !is_reg( t2 ) || !is_reg( t3 ) )
                    show_error( "final two arguments must be registers" );

                if ( !is_number( tokens[ 1 ] ) && !find_define( tokens[ 1 ] ) && ( T_INVALID != t1 ) ) 
                    show_error( "first argument must be an address" );

                val = 0;
                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                    val = number_or_define( tokens[ 2 ] );
                check_if_in_i16_range( val );

                code[ code_so_far++ ] = compose_op( 4, reg_from_token( t3 ), 3 );
                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 0 );
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_STO:
            {
                if ( 4 != token_count )
                    show_error( "sto takes 3 values: stob address[ roffset ], rsrc" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );

                if ( !is_reg( t2 ) || !is_reg( t3 ) )
                    show_error( "final two arguments must be registers" );

                if ( !is_number( tokens[ 1 ] ) && !find_define( tokens[ 1 ] ) && ( T_INVALID != t1 ) ) 
                    show_error( "first argument must be an address" );

                val = number_or_define( tokens[ 2 ] );
                check_if_in_i16_range( val );

                code[ code_so_far++ ] = compose_op( 4, reg_from_token( t3 ), 3 );
                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 1 );
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_STINCB:
            {
                if ( 3 != token_count )
                    show_error( "stincb takes a register and a value" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( !is_reg( t1 ) )
                    show_error( "first argument must be a register" );

                if ( !is_number( tokens[ 2 ] ) && !find_define( tokens[ 2 ] ) && ( T_INVALID != t2 ) ) 
                    show_error( "second argument must be a constant, define, or label" );

                val = 0;
                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                {
                    val = number_or_define( tokens[ 2 ] );
                    if ( val > 255 )
                        show_error( "stincb value must be < 256" );
                }

                code[ code_so_far++ ] = compose_op( 1, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = 0;
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_STINC:
            {
                if ( 3 != token_count )
                    show_error( "stinc takes a register and a value" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( !is_reg( t1 ) )
                    show_error( "first argument must be a register" );

                if ( !is_number( tokens[ 2 ] ) && !find_define( tokens[ 2 ] ) && ( T_INVALID != t2 ) ) 
                    show_error( "second argument must be a constant, define, or label" );

                val = 0;
                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                    val = number_or_define( tokens[ 2 ] );
                check_if_in_i16_range( val );

                code[ code_so_far++ ] = compose_op( 1, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( 0, 0, g_byte_len );
                initialize_word_value( & code_so_far, val );
                break;
            }
            case T_STB:
            {
                if ( 3 != token_count )
                    show_error( "st takes two register arguments" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t1 ) || !is_reg( t2 ) )
                    show_error( "registers expected for both arguments" );

                code[ code_so_far++ ] = compose_op( 5, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 0 );
                break;
            }
            case T_LD:
            {
                if ( token_count < 3 )
                    show_error( "ld takes at least two arguments" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( is_reg( t2 ) ) // ld rdst, [rsrc]
                {
                    code[ code_so_far++ ] = compose_op( 6, reg_from_token( t1 ), 1 );
                    code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), g_byte_len );
                }
                else // ld rdst, [ address (+ number) ]
                {
                    if ( T_INVALID != t2 )
                        show_error( "label expected as second argument" );
    
                    code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 2 );
                    initialize_image_value( & code_so_far, 0 );
                }
                break;
            }
            case T_LDB:
            {
                if ( token_count < 3 )
                    show_error( "ldb takes at least two arguments" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( is_reg( t2 ) ) // ldb rdst, [rsrc]
                {
                    if ( !is_reg( t1 ) || ( 3 != token_count ) )
                        show_error( "invalid arguments for ldb. expected ldb rdst, [rsrc]\n" );

                    code[ code_so_far++ ] = compose_op( 6, reg_from_token( t1 ), 1 );
                    code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 0 );
                }
                else // ldb rdst, [ address (+ number) ]
                {
                    if ( T_INVALID != t2 )
                        show_error( "address expected as second argument" );
    
                    code[ code_so_far++ ] = compose_op( 6, reg_from_token( t1 ), 3 );
                    code[ code_so_far++ ] = 0;
                    initialize_word_value( & code_so_far, 0 );
                }
                break;
            }
            case T_CSTF:
            {
                if ( 5 != token_count )
                    show_error( "cstf takes four arguments" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );
                t3 = find_token( tokens[ 3 ] );
                if ( !is_relation_token( t3 ) )
                    show_error( "relation expected as third argument" );

                offset = (int16_t) number_or_define( tokens[ 4 ] );
                if ( offset < -4 || offset > 3 )
                    show_error( "cstf requires 4 arguments: register, register, REL, and integer >= -4 and <= 3\n" );


                code[ code_so_far++ ] = compose_op( 7, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( relation_from_token( t3 ), reg_from_token( t2 ), 0 );
                initialize_word_value( & code_so_far, (uint8_t) ( 0xff & ( offset << 2 ) ) );
                break;
            }
            case T_J:
            {
                if ( 5 != token_count )
                    show_error( "j takes four arguments" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );
                t3 = find_token( tokens[ 3 ] );
                if ( !is_relation_token( t3 ) )
                    show_error( "relation expected as third argument" );
                t4 = find_token( tokens[ 4] );
                if ( T_INVALID != t4 )
                    show_error( "label expected as fourth argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( relation_from_token( t3 ), reg_from_token( t2 ), 0 );
                initialize_word_value( & code_so_far, 0 );
                break;
            }
            case T_JI:
            {
                if ( 5 != token_count )
                    show_error( "ji takes three arguments: ji rleft, 1..8, RELATION, ADDRESS" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );

                arg = number_or_define( tokens[ 2 ] );
                if ( arg < 1 || arg > 8 )
                    show_error( "a number 1..8 is expected as second argument" );

                t3 = find_token( tokens[ 3 ] );
                if ( !is_relation_token( t3 ) )
                    show_error( "relation expected as third argument" );
                t4 = find_token( tokens[ 4] );
                if ( T_INVALID != t4 )
                    show_error( "label expected as fourth argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( relation_from_token( t3 ), (uint16_t) ( arg - 1 ), 1 );
                initialize_word_value( & code_so_far, 0 );
                break;
            }
            case T_JRELB:
            {
                if ( 6 != token_count )
                    show_error( "jrelb takes 5 arguments: jrelb rleft, raddress, 0..255, RELATION, label (in range -128..127)" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );

                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t2 ) )
                    show_error( "register expected as second argument" );

                result = (int16_t) number_or_define( tokens[ 3 ] );
                if ( result < 0 || result > 255 )
                    show_error( "constant 0..255 expected as third argument" );

                t4 = find_token( tokens[ 4 ] );
                if ( !is_relation_token( t4 ) )
                    show_error( "relation expected as fourth argument" );

                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( relation_from_token( t4 ), reg_from_token( t2 ), 2 );
                initialize_word_value( & code_so_far, (uint8_t) result );
                break;
            }
            case T_LDIB:
            {
                if ( 3 != token_count )
                    show_error( "ldib takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected" );

                t2 = find_token( tokens[ 2 ] );
                if ( ! ( ( T_INVALID == t2 ) || is_number( tokens[ 2 ] ) || T_DEFINE == t2 ) )
                    show_error( "number, define, or label expected as second argument" );

                if ( is_number( tokens[ 2 ] ) )
                    ival = (int16_t) atoi( tokens[ 2 ] );
                else if ( define_exists( tokens[ 2 ] ) )
                    ival = get_define( tokens[ 2 ] );
                else
                    ival = 0; /* placeholder */

                if ( ival < -16 || ival > 15 )
                    show_error( "ldib only supports values -16..15" );

                code[ code_so_far++ ] = compose_op( 3, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = (uint8_t) ( ( 3 << 5 ) | (uint8_t) ( 0x1f & ival ) );
                break;
            }
            case T_LDI:
            {
                if ( 3 != token_count )
                    show_error( "ldi takes two arguments" );
                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected" );

                t2 = find_token( tokens[ 2 ] );
                if ( ! ( ( T_INVALID == t2 ) || is_number( tokens[ 2 ] ) || T_DEFINE == t2 ) )
                    show_error( "number, define, or label expected as second argument" );

                if ( is_number( tokens[ 2 ] ) )
                    ival = (int16_t) atoi( tokens[ 2 ] );
                else if ( define_exists( tokens[ 2 ] ) )
                    ival = get_define( tokens[ 2 ] );
                else
                    ival = 0; /* placeholder */

                code[ code_so_far++ ] = compose_op( 1, reg_from_token( t1 ), 2 );
                initialize_image_value( & code_so_far, ival );
                break;
            }
            case T_CMPST:
            {
                if ( 4 != token_count )
                    show_error( "cmpst takes 3 arguments: cmpst, r0dst, r1right, relation" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );

                if ( !is_reg( t1 ) || !is_reg( t2 ) || !is_relation_token( t3 ) )
                    show_error( "cmpst takes 3 arguments: cmpst, r0dst, r1right, relation" );

                code[ code_so_far++ ] = compose_op( 2, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( relation_from_token( t3 ), reg_from_token( t2 ), 0 );
                break;
            }
            case T_MATH:
            {
                if ( 5 != token_count )
                    show_error( "math takes 4 arguments: r0dst, r1left, r2right, MATH" );

                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );
                t4 = find_token( tokens[ 4 ] );

                if ( !is_reg( t1 ) || !is_reg( t2 ) || !is_reg( t3 ) || !is_math_token( t4 ) )
                    show_error( "math takes 4 arguments: r0dst, r1left, r2right, MATH" );

                code[ code_so_far++ ] = compose_op( 6, reg_from_token( t1 ), 3 );
                code[ code_so_far++ ] = compose_op( 2, reg_from_token( t2 ), 0 );
                initialize_word_value( & code_so_far, compose_op( math_from_token( t4 ), reg_from_token( t3 ), 0 ) );
                break;
            }
            case T_MATHST:
            {
                if ( 4 != token_count )
                    show_error( "mathst takes 3 arguments: cmpst, r0dst, r1right, math" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                t3 = find_token( tokens[ 3 ] );

                if ( !is_reg( t1 ) || !is_reg( t2 ) || !is_math_token( t3 ) )
                    show_error( "cmpst takes 3 arguments: cmpst, r0dst, r1right, relation" );

                code[ code_so_far++ ] = compose_op( 7, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( math_from_token( t3 ), reg_from_token( t2 ), 0 );
                break;
            }
            case T_MOV:
            {
                if ( 3 != token_count )
                    show_error( "mov takes 2 register arguments" );
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );
                if ( !is_reg( t1 ) || !is_reg( t2 ) )
                    show_error( "mov takes 2 register arguments" );

                code[ code_so_far++ ] = compose_op( 1, reg_from_token( t1 ), 1 );
                code[ code_so_far++ ] = compose_op( 0, reg_from_token( t2 ), 0 );
                break;
            }
            case T_INV:
            {
                if ( 2 != token_count )
                    show_error( "inv takes one argument - a register" );

                t1 = find_token( tokens[ 1 ] );
                if ( !is_reg( t1 ) )
                    show_error( "register expected as first argument" );

                code[ code_so_far++ ] = compose_op( 7, reg_from_token( t1 ), 0 );
                break;
            }
            default:
            {
                printf( "internal error; token %d '%s' not handled. ", (int) t, tokens[ 0 ] );
                show_error( "" );
                break;
            }
        }
    }

    fseek( fp, 0, SEEK_SET );

    if ( 1 == data_mode )
        show_error( "missing .dataend statement" );
    if ( 1 == code_mode )
        show_error( "missing .codeend statement" );

    /* second pass: append data and patch addresses */

    /* align code and initialized data to native width */

    code_so_far = round_up( code_so_far, g_image_width );
    initialized_data_so_far = round_up( initialized_data_so_far, g_image_width );

    data_mode = 0;
    code_mode = 0;
    total_code = code_so_far;
    code_so_far = 0;
    total_initialized_data = initialized_data_so_far;
    initialized_data_offset = total_code;
    zeroed_data_offset = total_code + total_initialized_data;

    code_so_far += g_image_width; /* get past initial word with halt instructions */

    line = 0;
    while ( fgets( buf, sizeof( buf ), fp ) )
    {
        line++;
        strcpy( original_line, buf );
        p = strchr( (char *) buf, ';' );
        if ( p )
            *p = 0;
        rm_white( buf );

        len = (width_t) strlen( buf );
        if ( (width_t) 0 == len )
            continue;

        token_count = tokenize( buf );

        if ( ':' == buf[ len - 1 ] )
            continue;

        if ( show_verbose_tracing )
        {
            printf( "second pass line %d has token count: %d -- %s\n", (int) line, token_count, buf );
            printf( "  code_so_far: %s, op0: %02x\n", render_width_t( code_so_far ), code[ code_so_far ] );
            for ( t = 0; t < (size_t) token_count; t++ )
                printf( "  token %d: '%s' has type %d == %s\n", (int) t, tokens[ t ],
                        find_token( tokens[ t ] ), TokenSet[ find_token( tokens[ t ] ) ] );
        }

        if ( offsets[ line ] != code_so_far )
        {
            printf( "offset expected %d, second pass is at %d\n", (int) offsets[ line ], (int) code_so_far );
            show_error( "internal error: offset in second pass doesn't match" );
        }

        t = find_token( tokens[ 0 ] );

        switch( t )
        {
            case T_DATA:
            case T_DATAEND:
            {
                data_mode++;
                break;
            }
            case T_CODE:
            case T_CODEEND:
            {
                code_mode++;
                break;
            }
            case T_DEFINE:
            {
                break;
            }
            case T_ALIGN:
            {
                if ( 2 == token_count )
                    alignment = number_or_define( tokens[ 1 ] );
                else
                    alignment = g_image_width;

                if ( 1 == data_mode )
                {
                    /* align both because we don't know what's next */
                    zeroed_data_offset = round_up( zeroed_data_offset, alignment );
                    initialized_data_offset = round_up( initialized_data_offset, alignment );
                }
                else
                    code_so_far = round_up( code_so_far, alignment );
                break;
            }
            case T_BYTE:
            case T_WORD:
            case T_IMAGE_T:
            {
                plabel = find_label( tokens[ 1 ] );
                if ( !plabel )
                    show_error( "internal error: can't find label on second pass" );
                if ( (width_t) 0 != plabel->offset )
                    show_error( "internal error in second pass: offset data isn't zero" );

                plabel->offset = zeroed_data_offset;
                zeroed_data_offset += plabel->datasize;
                break;
            }
            case T_STRING:
            {
                plabel = find_label( tokens[ 1 ] );
                if ( !plabel )
                    show_error( "internal error: can't find label on second pass" );

                plabel->offset = initialized_data_offset;
                memcpy( & code[ initialized_data_offset ], tokens[ 2 ], (int) plabel->datasize );
                initialized_data_offset += plabel->datasize;
                break;
            }
            case T_LDAE:
            {
                code_so_far++;
                plabel = find_label( tokens[ 1 ] );
                if ( 0 != code[ code_so_far ] || 0 != code[ code_so_far + 1 ] )
                    show_error( "internal error in second pass: offset isn't zero" );

                initialize_image_value( & code_so_far, plabel->offset );
                break;
            }
            case T_JMP:
            {
                // forms of jmp:
                //     jmp register (address 0 implied)
                //     jmp address (rzero implied)
                //     jmp address, register

                t1 = find_token( tokens[ 1 ] );
                val = 0;
                reg = 0; // rzero

                if ( !is_reg( t1 ) )
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                }    

                code_so_far++;
                initialize_image_value( & code_so_far, val );
                break;
            }
            case T_CALL:
            {
                // forms of call:
                //     call register (address 0 implied)
                //     call address (rzero implied)
                //     call address, register

                t1 = find_token( tokens[ 1 ] );
                val = 0;
                reg = 0; // rzero

                if ( !is_reg( t1 ) )
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                }    

                code_so_far++;
                if ( 3 == token_count )
                {
                    code_so_far++;
                    word_zero_check( code_so_far );
                    diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                    check_if_in_i16_range( (iwidth_t) diff );
                    initialize_word_value( & code_so_far, diff );
                }
                else
                {
                    width_zero_check( code_so_far );
                    initialize_image_value( & code_so_far, val );
                }
                break;
            }
            case T_CALLNF:
            {
                // forms of call:
                //     call register (address 0 implied)
                //     call address (rzero implied)
                //     call address, register

                t1 = find_token( tokens[ 1 ] );
                val = 0; // default address is 0
                reg = 0; // rzero

                if ( !is_reg( t1 ) )
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                }    

                code_so_far += 2;

                word_zero_check( code_so_far );
                diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                check_if_in_i16_range( (iwidth_t) diff );
                initialize_word_value( & code_so_far, diff );
                break;
            }
            case T_INC:
            case T_DEC:
            {
                // forms of inc:
                //     inc register
                //     inc [register]
                //     inc [address]     (rzero implied)
                //     inc [address, register]

                is_register = ( 0 == strchr( original_line, '[' ) );
                t1 = find_token( tokens[ 1 ] );

                if ( is_register )
                    code_so_far++;
                else
                {
                    val = 0;
                    if ( !is_reg( t1 ) )
                    {
                        plabel = find_label( tokens[ 1 ] );
                        val = plabel->offset;
                    }

                    code_so_far++;
                    width_zero_check( code_so_far );
                    initialize_image_value( & code_so_far, val );
                }
                break;
            }
            case T_ST:
            {
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( is_reg( t1 ) ) // st [rdst], rsrc
                    code_so_far += 2;
                else // st [address], register
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                    code_so_far++;
                    width_zero_check( code_so_far );
                    initialize_image_value( & code_so_far, val );
                }
                break;
            }
            case T_LD:
            case T_LDB:
            {
                t1 = find_token( tokens[ 1 ] );
                t2 = find_token( tokens[ 2 ] );

                if ( is_reg( t2 ) ) // ld rdst, [rsrc]
                    code_so_far += 2;
                else // ld register, [ address ( + number ) ]
                {
                    plabel = find_label( tokens[ 2 ] );
                    val = plabel->offset;
                    code_so_far++;
                    if ( 5 == token_count )
                    {
                        t3 = find_token( tokens[ 3 ] );
                        if ( T_PLUS == t3 && is_number( tokens[ 4 ] ) )
                            val += (uint16_t) atoi( tokens[ 4 ] );
                        else
                            show_error( "syntax error with ld address. use ld reg, [ address + offset ]" );
                    }

                    if ( T_LDB == t )
                    {
                        code_so_far++;
                        word_zero_check( code_so_far );
                        diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                        check_if_in_i16_range( (iwidth_t) diff );
                        initialize_word_value( & code_so_far, diff );
                    }
                    else
                    {
                        width_zero_check( code_so_far );
                        initialize_image_value( & code_so_far, val );
                    }
                }
                break;
            }
            case T_J:
            case T_JI:
            {
                if ( 5 != token_count )
                    show_error( "j and ji take two arguments" );

                plabel = find_label( tokens[ 4 ] );
                val = plabel->offset;

                code_so_far += 2;
                word_zero_check( code_so_far );
                diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                check_if_in_i16_range( (iwidth_t) diff );
                initialize_word_value( & code_so_far, diff );
                break;
            }
            case T_JRELB:
            {
                if ( !strcmp( tokens[ 5 ], "ret" ) )
                    code_so_far += ( 2 + g_image_width );
                else
                {
                    if ( !strcmp( tokens[ 5 ], "retnf" ) )
                        diff = 1;
                    else
                    {
                        plabel = find_label( tokens[ 5 ] );
                        val = plabel->offset;
        
                        if ( val > code_so_far )
                            diff = val - code_so_far;
                        else
                            diff = code_so_far - val;
        
                        if ( diff > 127 || diff < -128 )
                            show_error( "jrel jump offset must be -128..127" );
                    }
    
                    code_so_far += 3;
                    if ( 0 != code[ code_so_far ] )
                        show_error( "internal error in second pass: offset isn't zero" );
    
                    code[ code_so_far++ ] = (int8_t) diff;
                }
                break;
            }
            case T_LDIB:
            {
                code_so_far++;

                if ( !is_number( tokens[ 2 ] ) && !find_define( tokens[ 2 ] ) )
                {
                    plabel = find_label( tokens[ 2 ] );
                    ival = (int16_t) plabel->offset;
                    if ( ival < -16 || ival > 15 )
                        show_error( "ldib only supports values -16..15" );
                    if ( 0 != code[ code_so_far ] || 0 != code[ code_so_far + 1 ] )
                        show_error( "internal error in second pass: offset isn't zero" );
                    code[ code_so_far++ ] |= (uint8_t) ival;
                }
                else
                    code_so_far++;
                break;
            }
            case T_LDI:
            {
                code_so_far++;

                if ( !is_number( tokens[ 2 ] ) && !find_define( tokens[ 2 ] ) )
                {
                    plabel = find_label( tokens[ 2 ] );
                    val = plabel->offset;
                    width_zero_check( code_so_far );
                    initialize_image_value( & code_so_far, val );
                }
                else
                    code_so_far += g_image_width;
                break;
            }
            case T_LDOINCB:
            case T_LDOINC:
            case T_LDOB:
            case T_LDO:
            {
                t2 = find_token( tokens[ 2 ] );
                code_so_far += 2;
                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                    code_so_far += 2;
                else
                {
                    plabel = find_label( tokens[ 2 ] );
                    val = plabel->offset;
                    word_zero_check( code_so_far );
                    diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                    check_if_in_i16_range( (iwidth_t) diff );
                    initialize_word_value( & code_so_far, diff );
                }
                break;
            }
            case T_STOB:
            case T_STO:
            {
                t1 = find_token( tokens[ 1 ] );
                code_so_far += 2;
                if ( is_number( tokens[ 1 ] ) || find_define( tokens[ 1 ] ) )
                    code_so_far += 2;
                else
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                    word_zero_check( code_so_far );
                    diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                    check_if_in_i16_range( (iwidth_t) diff );
                    initialize_word_value( & code_so_far, diff );
                }
                break;
            }
            case T_STI:
            case T_STIB:
            {
                code_so_far += 2;
                val = 0;

                if ( is_number( tokens[ 1 ] ) || find_define( tokens[ 1 ] ) )
                    code_so_far += g_image_width;
                else
                {
                    plabel = find_label( tokens[ 1 ] );
                    val = plabel->offset;
                    word_zero_check( code_so_far );
                    diff = (iwidth_t) val - (iwidth_t) ( code_so_far - 2 );
                    check_if_in_i16_range( (iwidth_t) diff );
                    initialize_word_value( & code_so_far, diff );
                }
                break;
            }
            case T_STINC:
            case T_STINCB:
            {
                code_so_far += 2;
                t2 = find_token( tokens[ 2 ] );
                val = 0;

                if ( is_number( tokens[ 2 ] ) || find_define( tokens[ 2 ] ) )
                    code_so_far += 2;
                else
                {
                    plabel = find_label( tokens[ 2 ] );
                    val = plabel->offset;

                    if ( ( T_STINCB == t ) && ( val > 255 ) )
                        show_error( "stincb requires numbers 0..255" );

                    check_if_in_i16_range( (iwidth_t) val );
                    initialize_word_value( & code_so_far, val );
                }
                break;
            }
            case T_CSTF:
            case T_MATH:
            {
                code_so_far += 4;
                break;
            }
            case T_LDF:
            case T_STF:
            case T_CMPST:
            case T_MATHST:
            case T_MOV:
            case T_ADD:
            case T_MUL:
            case T_MODDIV:
            case T_DIV:
            case T_SYSCALL:
            case T_PUSHF:
            case T_STST:
            case T_ADDIMGW:
            case T_SUBIMGW:
            case T_STB:
            case T_MEMF:
            case T_MEMFB:
            case T_STADDB:
            case T_SIGNEXB:
            case T_SIGNEXW:
            case T_SIGNEXDW:
            {
                code_so_far += 2;
                break;
            }
            case T_RET:
            {
                code_so_far++;

                if ( 2 == token_count )
                    code_so_far++;
                break;
            }
            case T_IMULST:
            case T_IDIVST:
            case T_ADDST:
            case T_ZERO:
            case T_RETZERO:
            case T_RETNF:
            case T_RETZERONF:
            case T_PUSH:
            case T_POP:
            case T_SHL:
            case T_SHLIMG:
            case T_SHR:
            case T_SHRIMG:
            case T_INV:
            case T_SUBST:
            case T_IMGWID:
            {
                code_so_far++;
                break;
            }
            default: { show_error( "internal error; token not handled" ); };
        }
    }

    fclose( fp );

    if ( create_listing )
    {
        strcpy( aclistfile, acfile );
        p = strstr( aclistfile, ".s" );
        strcpy( p, ".lst" );

#ifdef MSC6 /* w+ doesn't truncate existing files with this compiler */
        remove( aclistfile );
#endif
        fp = fopen( aclistfile, "w+" );
        if ( !fp )
            show_error( "can't open listing file" );

        fprintf( fp, ".code\n" );
        x = 0;
        while ( x < code_so_far ) /* use the non-rounded-up code length or halt instructions appear at the end */
        {
            pc = lookup_label( (uint32_t) x );
            if ( pc )
                fprintf( fp, "%s:\n", pc );

            pc = DisassembleOI( code + x, (oi_t) x, g_image_width );
            if ( !*pc )
            {
                printf( "can't disassemble opcode %02x, %02x\n", code[ x ], code[ x + 1 ] );
                show_error( "internal error" );
            }
            fprintf( fp, "    %08x", (unsigned int) x );
            len = 1 + ( code[ x ] & 3 );
            if ( (width_t) 3 == len )
                len += ( g_image_width - 2 );
            fprintf( fp, "    %s", pc );

            l = strlen( pc );
            if ( l < 37 )
                print_space( fp, 37 - l );
            fprintf( fp, " ; " );
            for ( j = 0; j < len; j++ )
                fprintf( fp, "%02x ", code[ x + j ] );
            fprintf( fp, "\n" );
            x += len;
        }

        fprintf( fp, ".codeend\n" );
        fprintf( fp, ".data\n" );
        do
        {
            pc = lookup_label( (uint32_t) x );
            if ( !pc )
                x++; /* alignment skip */
            else
            {
                plabel = find_label( pc );
                fprintf( fp, "%s:\n", plabel->plabel );
                fprintf( fp, "    %08x  ; %u bytes\n", (unsigned int) x, (unsigned int) plabel->datasize );
    
                if ( plabel->initialized )
                {
                    for ( j = 0; j < plabel->datasize; j++ )
                        fprintf( fp, "%02x ", code[ x + j ] );
                    fprintf( fp, "\n" );
                }
    
                x += plabel->datasize;
            }
        } while( x < ( total_code + total_initialized_data + total_zeroed_data ) );

        fprintf( fp, ".dataend\n" );
        fclose( fp );
    }

#if 0
    printf( "symbols:\n" );
    printf( "    size     offset    name\n" );
    for ( t = 0; t < g_cLabels; t++ )      //if ( g_pLabels[ t ]->datasize > 0 )
        printf( "    %04x     %04x      %s\n", (unsigned int) g_pLabels[ t ]->datasize, (unsigned int) g_pLabels[ t ]->offset, g_pLabels[ t ]->plabel );
#endif

    if ( show_image_info )
    {
        uint32_t aCounts[ 4 ] = { 0, 0, 0, 0 };
        x = g_image_width; /* skip syscall address  */
        while ( x < total_code )
        {
            len = code[ x ] & 3;
            aCounts[ len ]++;
            if ( (width_t) 2 == len )
                x += ( 1 + g_image_width );
            else if ( (width_t) 3 == len )
                x += ( 2 + g_image_width );
            else
                x += ( 1 + len );
        }

        printf( "instruction usage by length:\n" );
        for ( x = 0; x < 4; x++ )
            printf( "    %u bytes:  %u\n", (unsigned int) ( 1 + x ), aCounts[ x ] );
    }

    p = strstr( acfile, ".s" );
    strcpy( p, ".oi" );

#ifdef MSC6 /* w+b doesn't truncate existing files with this compiler */
    remove( acfile );
#endif

    fp = fopen( acfile, "w+b" );
    if ( !fp )
        show_error( "can't open output file" );

    memset( &h, 0, sizeof( h ) );
    h.sig0 = 'O';
    h.sig1 = 'I';
    h.version = 1;
    h.flags = 0;
    if ( 4 == g_image_width )
        h.flags |= 1;
    else if ( 8 == g_image_width )
        h.flags |= 2;
    h.unused = 0;
    h.cbCode = (uint32_t) total_code;
    h.cbInitializedData = (uint32_t) total_initialized_data;
    h.cbZeroFilledData = (uint32_t) total_zeroed_data;
    h.cbStack = 0x80 * g_image_width; 
    h.loRamRequired = h.cbCode + h.cbInitializedData + h.cbZeroFilledData + h.cbStack;
    h.loInitialPC = g_image_width; /* first image width is the address of the syscall function or 0/halt */
    fwrite( &h, sizeof( h ), 1, fp );

    fwrite( code, (int) ( total_code + total_initialized_data ), 1, fp );
    fclose( fp );

    if ( show_image_info )
    {
        printf( "oi header:\n" );
        printf( "  signature:                %c%c\n", h.sig0, h.sig1 );
        printf( "  version:                  %u\n", h.version );
        printf( "  flags:                    %04xh\n", h.flags );
        printf( "  ram required:             %u\n", h.loRamRequired );
        printf( "  code size:                %u\n", h.cbCode );
        printf( "  initialized data size:    %u\n", h.cbInitializedData );
        printf( "  zero-filled data size:    %u\n", h.cbZeroFilledData );
        printf( "  stack size:               %u\n", h.cbStack );
        printf( "  initial PC:               %u\n", h.loInitialPC );
    }

    return 0;
} /* main */
