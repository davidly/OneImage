/*
    OneImage bytecode interpreter.
    Written by David Lee in August 2024.

    The OneImage instruction set is designed for:
        -- efficiency of power consumption, execution time, and RAM usage
        -- ease of targeting for compilers
        -- support a single assembler source file across 16, 32, and 64 bit execution environments
        -- taking the best instructions from many different ISAs and bytecode interpreters

    Native Width: number of bytes for registers, pointers, etc. 16, 32, or 64. Build with defines OI2, OI4, or OI8

    Image Width: number of bytes for registers, pointers, etc. in the .oi executable image.
    16, 32, or 64 as set in the flags field of the image header.

    For now, Native Width must be the same as Image Width. To do: enable NW >= IW.

    Operations: opcode bytes. op = first byte, op1 = next byte, op2 = folllowing byte

    Address 0:
        Address 0 contains a pointer to the syscall function.
        In the emulator, this is set to 0 and syscalls are handled directly.
        A return to address 0 has the same effect as a halt instruction.

    Instruction length (stored in bits 1..0): # of bytes in the opcode minus 1.
        This number is for 16-bit image width. For 32 and 64 bit image width, the
        instruction length for 3 and 4 byte len increases from 2 to 4 or 8 byte values.

    Registers (stored in bits 4..2):
        3 bit IDs 0..7 for rzero, rpc, rsp, rframe, rarg1, rarg2, rres, rtmp
        referred to reg0, reg1, etc. for the reg field from op, op1, ...

    Function (stored in bits 7..5)
        Maps to Math, Relations, Operation, upper 3 bits of immediate
        referred to funct0, funct1, etc. for the funct field from op, op1, ...

    Math (stored in bits 7..5):
        3 bit IDs 0..7 for add, sub, imul, idiv, or, xor, and, cmp

    Relation (stored in bits 7..5) 
        3 bit IDs 0..7 for gt, lt, eq, ne, ge, le

    Operation: Another name for Function when it's not Math or Relation

    Width
        2 lower bits 1..0 of byte 1 of 2 and 4 byte operations. 0 = byte, 1 = word, 2 = dword, 3 = qword
        when width doesn't apply to an instruction, this field can instead differentiate between instructions

    4 byte operations: (stored in bits 7..5 of first byte of four-byte operations):
      note: all addresses are pc-relative signed 16-bit values
        0:  j / ji / jrel / jrelb   j if width = 0
                                    ji if width = 1
                                    jrelb if width = 2
                                    jrel if width = 3
                               j r0left, r1right, RELATION, ADDRESS
                               ji r0left, r1rightIMMEDIATE, RELATION, ADDRESS.
                                   r1rightIMMEDIATE is unsigned value + 1. == 1..8
                               jrel r0left, r1rightADDRESS, offset (from r1right), RELATION (-128..127 pc offset)
                                   for jrel, the address offset is unsigned. the pc offset is signed.
                                   pc offset special cases:
                                       0: ret
                                       1: retnf
        1:  stinc / stincb:    [ r0off ] = value; inc r0off per width
        2:  ldinc / ldincb:    r0dst = address[ r1off ]. inc r1off per width.
        3:  value of funct in op1:
            0:   call:              functiontableaddress[ r0 ]
            1:   callnf:            functiontableaddress[ r0 ]   (don't push or setup rframe).
            2:   callnf:            address + r0   (don't push or setup rframe).
        4:  sto / stob:        address[ r1offset ] = r0value. offset is multiplied by width
        5:  value of funct in op1
            0:   ldo / ldob:          r0dst = address[ r1off ]. offset is multiplied by width
            1:   ldoinc / ldoincb:    r1++ then r0dst = address[ r1off ]. offset is multiplied by width
            2:   ldi constant -32768..32767 sign extended. use ldb if possible and ldi's 3-byte form if the number is large or a 2-byte native width
        6:  value of funct in op1
            0:   ld / ldb:     r0dst = [ address ]
            1:   sti / stib:   [address ], r1 -8..7
            2:   math r0dst, r1src, r2src, math
        7:  cstf               conditional stack frame store: cstf r0left r1right frame1REL reg2FRAMEOFFSET

    Opcode lengths:
        4 byte operations:
                    7..5        4..2            1..0
            byte 0: operation   Registers lhs   3
            byte 1: Relations   Registers rhs   Width
            bytes 2/3: 16 bit value
            opcode is 4 bytes unless ADDRESS is used (funct0 2, 3, 4, 5, 6), in which case address is image width
                           
        3 byte operations: high 3 bits 0..7:  ld, ldi, st, jmp, inc, dec, ldae, call
            ldae target is always rres. register is multiplied by image width and added to address for read. load array entry.
            following the first byte are image width bytes: 2, 4, 8

        2 byte operations:
                    7..5                 4..2            1..0
            byte 0: Operation            r0 lhs          01
                0: Math r0dst r1src
                1: cmov r0dst, r1src, Relation. if ( r0dst Relation r1src ) r0dst = rssrc.
                   mov r0dst, r1src     -- this maps to cmov r0dst, r1src, ne
                2: cmpst r0dst, r1src, Relation -- r0dst = ( pop() Relation r1src ). sets r0dst to a boolean 1 or 0
                3: - 0/1 funct: ldf/stf. loads and stores r0 relative to rframe. r1 >= 0 is is frame[ ( 3 + r1 ) * 2 ]. r1 < 0 is frame[ ( 1 + r1 ) * 2 ]
                   - 2 funct: ret x     -- pop x items off the stack and return
                   - 3 funct: ldib x    -- load immediate small signed values
                   - 4 funct: signex    -- sign extend the specified width to the native width
                   - 5 funct: memf. rarg1 = address, rarg2 = # of items to copy. rtmp = value to copy. -- memfill
                   - 6 funct: stadd. stb [rtmp+rarg1] = 0. add rtmp, rarg2. loop if rtmp le rarg1 -- store and add in a loop
                   - 7 funct: moddiv. r0 = r0 % r1. push( r0 / r1 ). -- calculate both mod and div
                4:
                   - 0 funct: syscall ( ( reg of byte 0 << 3 ) | ( reg of byte 1 ) ). 6 bit system ID. bit width must be 0.
                   - 1 funct: pushf reg1CONSTANT. r1 >= 0 is is frame[ ( 3 + r1 ) * 2 ]. r1 < 0 is frame[ ( 1 + r1 ) * 2 ]
                   - 2 funct: stst reg0.  i.e.  st [pop()], reg0
                   - 3 funct: width 0: addimgw reg0
                              width 1: subimgw reg0
                   - 4 funct: stinc [reg0], reg1 then increment reg0 by width of store
                   - 5 funct: swap reg0, reg1
                5: st [r0dst] r1src     later: lots of free functs. make conditionals?
                6: ld r0dst [r1src]     later: lots of free functs. make contitionals?
                7: mathst r0dst, r1src, Math   -- r0dst = ( pop() MATH r1src )
            byte 1: Math/Relation/Funct  r1 rhs          bit width ( 0=8, 1=16...)

        1 byte operations: high 3 bits 0..7 operate on reg0:   inc, dec, push, pop, zero, shl, shr, inv
            exceptions:          overridden
                00 halt       -- inc rzero
                08 retzero    -- inc rsp
                20 imulst     -- dec rzero
                28 shlimg     -- dec rsp. shift rres left based on image width 2=>1, 4=>2, 8=>3
                40 push rzero -- DON'T OVERRIDE (idivst)
                48 retzeronf  -- push rsp (nf = no rframe restoration)
                60 pop rzero  -- DON'T OVERRIDE
                68 retnf      -- pop rsp (nf = no rframe restoration)
                80 subst      -- zero rzero
                84 imgwid     -- zero rpc. sets rres to the image width
                88 shrimg     -- zero rsp
                a0 addst      -- shl rzero
                a4 UNUSED     -- shl rpc
                a8 idivst     -- shl rsp
                c0 ret        -- shr rzero
                c4 UNUSED     -- shr rpc
                c8 UNUSED     -- shr rsp
                e0 andst      -- inv rzero
                e4 UNUSED     -- inv rpc
                e8 UNUSED     -- inv rsp

    Note: there are extra, unnecessary casts to appease older compilers. Also, no #elif or defined() exists on older compilers.

    Tested compilers:
        - CP/M 2.2 on 8080 and Z80:
              - HiSoft HI-TECH C v3.09
              - MANX Aztec C v1.06D
        - DOS 3.x on 8086:
              - Microsoft C v6.00AX
              - Open Watcom C/C++ x86 16-bit Compile and Link Utility Version 2.0 beta Oct  9 2023 02:19:55 (64-bit)
        - Windows 11 on x64:
              - Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33814 for x64
              - Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33814 for x86
              - g++ (Rev3, Built by MSYS2 project) 13.2.0
        - Ubuntu 22.04.3 on x64
              - g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

    Build notes:
        - One of OI2, OI4, or OI8 must be defined when compiling
        - The resulting oios executable can only run .oi binaries with image-width that matches.
        - With the 8-bit compilers I can only get OI2 builds to work
        - With the 16-bit compilers only OI2 and OI4 work.
        - With 32-bit and 64-bit compilers all widths work.
*/

#include <stdio.h>

#ifndef AZTECCPM
#include <assert.h>
#include <string.h>

#ifndef MSC6
#include <stdint.h>
#endif
#endif

#include "oi.h"
#include "trace.h"

#ifdef AZTECCPM
#define memset( p, val, len ) setmem( p, len, val )
#endif

#define true 1
#define false 0

#define OI_FLAG_TRACE_INSTRUCTIONS 1

struct OneImage g_oi;

#ifdef OLDCPU /* CP/M machines with 64k or less total ram */
static uint8_t ram[ 32767 ];
#else
#ifdef MSC6 /* DOS version is built with small memory model */
static uint8_t ram[ 60000 ];
#endif
static uint8_t ram[ 8 * 1024 * 1024 ]; /* arbitrary */
#endif

#ifndef NDEBUG
static uint8_t g_OIState = 0;
#ifdef OLDCPU
void TraceInstructionsOI( t ) bool t;
#else
void TraceInstructionsOI( bool t )
#endif
{ if ( t ) g_OIState |= OI_FLAG_TRACE_INSTRUCTIONS; else g_OIState &= ~OI_FLAG_TRACE_INSTRUCTIONS; }

#endif

/* this choice is for performance on various platforms */

#ifdef OLDCPU
    typedef uint8_t opcode_t;
#else
    typedef size_t opcode_t;
#endif

/* these macros lose safety and readability over inlined functions, but for older compilers it's much faster */

#define get_byte( address ) ( ram[ address ] )
#define set_byte( address, val ) ( ram[ address ] = val )

#define get_word( address ) ( * (uint16_t *) ( ram + address ) )
#define set_word( address, val ) ( * (uint16_t *) ( ram + address ) = val )

#define get_oiword( address ) ( * (oi_t *) ( ram + address ) )
#define set_oiword( address, val ) ( * (oi_t *) ( ram + address ) = val )

#define get_dword( address ) ( * (uint32_t *) ( ram + address ) )
#define set_dword( address, val ) ( * (uint32_t *) ( ram + address ) = val )

#ifdef OI8
#define get_qword( address ) ( * (uint64_t *) ( ram + address ) )
#define set_qword( address, val ) ( * (uint64_t *) ( ram + address ) = val )
#endif

#ifdef OI8
#define get_preg_from_op( op ) ( (oi_t *) ( ( (uint8_t *) & g_oi.rzero ) + ( ( op << 1 ) & 0x38 ) ) )
#else
#define get_preg_from_op( op ) ( (oi_t *) ( ( (uint8_t *) & g_oi.rzero ) + ( op & 0x1c ) ) )
#endif

#define get_reg_from_op( op ) ( * get_preg_from_op( op ) )
#define set_reg_from_op( op, val ) ( * get_preg_from_op( op ) = val )

#define add_reg_from_op( op, val ) ( ( * get_preg_from_op( op ) ) += val )

#define inc_reg_from_op( op ) ( ( * get_preg_from_op( op ) )++ )
#define dec_reg_from_op( op ) ( ( * get_preg_from_op( op ) )-- )

#define get_op() ( get_byte( g_oi.rpc ) )
#define get_op1() ( get_byte( g_oi.rpc + 1 ) )
#define get_op2() ( get_byte( g_oi.rpc + 2 ) )

#define push( val ) g_oi.rsp -= sizeof( oi_t ), set_oiword( g_oi.rsp, val )
#define pop_empty() g_oi.rsp += sizeof( oi_t )
#define pop( result ) result = get_oiword( g_oi.rsp ), pop_empty()

#ifdef OI2
#define if_1_is_width
#else
#define if_1_is_width if ( 1 == width )
#endif

#ifdef OI4
#define if_2_is_width
#else
#define if_2_is_width if ( 2 == width )
#endif

#ifdef OLDCPU
void ResetOI( memSize, pc, sp, imageWidth ) oi_t memSize; oi_t pc; oi_t sp; uint8_t imageWidth;
#else
void ResetOI( oi_t memSize, oi_t pc, oi_t sp, uint8_t imageWidth )
#endif
{
    memset( &g_oi, 0, sizeof( g_oi ) );
    memset( ram, 0, (size_t) memSize );
    g_oi.rpc = pc;
    g_oi.rsp = sp;
    g_oi.image_width = imageWidth;
    push( 0 );  /* rframe */
    push( 0 );  /* return address is 0, which has a halt instruction */
    g_oi.rframe = g_oi.rsp - sizeof( oi_t ); /* point frame at first local variable (if any) */
} /* ResetOI */

#ifdef OLDCPU
uint32_t RamInformationOI( required, ppRam ) uint32_t required; uint8_t ** ppRam;
#else
uint32_t RamInformationOI( uint32_t required, uint8_t ** ppRam )
#endif
{
    uint32_t available;

    available = (uint32_t) sizeof( ram );
    if ( ( 2 == sizeof( oi_t ) ) && ( available > 65536 ) )
        available = 65536;

    if ( available >= required )
        *ppRam = ram;
    else
        *ppRam = 0;
    return available;
} /* RamInformationOI */

#ifdef OLDCPU
oi_t CheckRelation( l, r, relation ) ioi_t l; ioi_t r; uint8_t relation;
#else
oi_t CheckRelation( ioi_t l, ioi_t r, uint8_t relation )
#endif
{
    __assume( relation <= 5 );
    switch ( relation )
    {
        case 0: { return ( l > r ); }
        case 1: { return ( l < r ); }
        case 2: { return ( l == r ); }
        case 3: { return ( l != r ); }
        case 4: { return ( l >= r ); }
        case 5: { return ( l <= r ); }
        default: { __assume( false ); }
    }

    assert( false );
/*
#ifndef HISOFTCPM
    return false;
#endif
*/
} /* CheckRelation */

#ifdef OLDCPU
oi_t Math( l, r, math ) oi_t l; oi_t r; uint8_t math;
#else
oi_t Math( oi_t l, oi_t r, uint8_t math )
#endif
{
    __assume( math <= 7 );
    switch ( math )
    {
        case 0: { return l + r; }
        case 1: { return l - r; }
        case 2: { return (ioi_t) l * (ioi_t) r; }
        case 3: { return (ioi_t) l / (ioi_t) r; }
        case 4: { return l | r; }
        case 5: { return l ^ r; }
        case 6: { return l & r; }
        case 7: { return l != r; }         /* true if !=, false if =. ( 0 != ( left - right ) ) */
        default: { __assume( false ); }
    }

    assert( false );
#ifndef HISOFTCPM
    return 0;
#endif
} /* Math */

#ifdef NDEBUG
#define illegal_instruction( a, b )
#else

#ifdef OLDCPU
static char * render_value( val, width ) oi_t val; uint8_t width;
#else
static const char * render_value( oi_t val, uint8_t width )
#endif
{
    static char ac[ 20 ];
    ac[ 0 ] = 0;

    if ( 1 == width )
        sprintf( ac, "%02x", (uint8_t) val );
    else if ( 2 == width )
        sprintf( ac, "%04x", (uint16_t) val );
#ifndef OI2
    else if ( 4 == width )
        sprintf( ac, "%08x", (uint32_t) val );
#ifdef OI8
    else
        sprintf( ac, "%016llx", val );
#endif
#endif

    return ac;
} /* render_value */

void TraceStateOI()
{
    trace( "%s", DisassembleOI( ram + g_oi.rpc, g_oi.rpc, g_oi.image_width ) );
    trace( "rzero:  %s\n", render_value( g_oi.rzero, sizeof( oi_t ) ) );
    trace( "rpc:    %s\n", render_value( g_oi.rpc, sizeof( oi_t ) ) );
    trace( "rsp:    %s\n", render_value( g_oi.rsp, sizeof( oi_t ) ) );
    trace( "rframe: %s\n", render_value( g_oi.rframe, sizeof( oi_t ) ) );
    trace( "rarg1:  %s\n", render_value( g_oi.rarg1, sizeof( oi_t ) ) );
    trace( "rarg2:  %s\n", render_value( g_oi.rarg2, sizeof( oi_t ) ) );
    trace( "rres:   %s\n", render_value( g_oi.rres, sizeof( oi_t ) ) );
    trace( "rtmp:   %s\n", render_value( g_oi.rtmp, sizeof( oi_t ) ) );
} /* TraceStateOI */

void illegal_instruction( size_t op, size_t op1 )
{
    printf( "illegal instruction. op %02x, op1 %02x\n", (uint8_t) op, (uint8_t) op1 );
    TraceStateOI();
    OIHardTermination();
} /* illegal_instruction */

static void TraceState()
{
    uint8_t op, op1, op2, op3;
    oi_t tos;
    const char * pdis;
    uint8_t * popcodes = ram + g_oi.rpc;

    op = popcodes[ 0 ];
    op1 = popcodes[ 1 ];
    op2 = popcodes[ 2 ];
    op3 = popcodes[ 3 ];
    tos = get_oiword( g_oi.rsp );

#ifdef OI2
    trace( "rpc %04x %02x %02x %02x %02x rres %x rtmp %x rarg1 %x rarg2 %x rframe %x, rsp %x tos %x : ",
            g_oi.rpc, op, op1, op2, op3, g_oi.rres, g_oi.rtmp, g_oi.rarg1, g_oi.rarg2, g_oi.rframe, g_oi.rsp, tos );
#endif
#ifdef OI4
#ifdef MSC6
    trace( "rpc %08lx %02x %02x %02x %02x rres %lx rtmp %lx rarg1 %lx rarg2 %lx rframe %lx, rsp %lx tos %lx : ",
            g_oi.rpc, op, op1, op2, op3, g_oi.rres, g_oi.rtmp, g_oi.rarg1, g_oi.rarg2, g_oi.rframe, g_oi.rsp, tos );
#else
    trace( "rpc %08x %02x %02x %02x %02x rres %x rtmp %x rarg1 %x rarg2 %x rframe %x, rsp %x tos %x : ",
            g_oi.rpc, op, op1, op2, op3, g_oi.rres, g_oi.rtmp, g_oi.rarg1, g_oi.rarg2, g_oi.rframe, g_oi.rsp, tos );
#endif
#endif
#ifdef OI8
    trace( "rpc %08llx %02x %02x %02x %02x rres %llx rtmp %llx rarg1 %llx rarg2 %llx rframe %llx, rsp %llx tos %llx : ",
            g_oi.rpc, op, op1, op2, op3, g_oi.rres, g_oi.rtmp, g_oi.rarg1, g_oi.rarg2, g_oi.rframe, g_oi.rsp, tos );
#endif

    pdis = DisassembleOI( popcodes, g_oi.rpc, sizeof( oi_t ) );
    if ( ! *pdis )
        illegal_instruction( op, op1 );

    trace( "%s\n", pdis );
} /* TraceState */
#endif

/* memf: rarg1 = array address, rarg2 = # of items (based on width) to fill. rtmp = value to copy. rres = first element to fill */

static void memfb_do()
{
    memset( & ( ram[ g_oi.rarg1 + g_oi.rres ] ), (uint8_t) g_oi.rtmp, (size_t) g_oi.rarg2 );
} /* memfb_do */

static void memfw_do()
{
    uint16_t * pw, * pbeyond;
    pw = (uint16_t *) & ram[ g_oi.rarg1 ];
    pw += g_oi.rres;
    pbeyond = pw + g_oi.rarg2;
    while ( pw != pbeyond )
        *pw++ = (uint16_t) g_oi.rtmp;
} /* memfw_do */

#ifndef OI2

static void memfdw_do()
{
    uint32_t * p, * pbeyond;
    p = (uint32_t *) & ram[ g_oi.rarg1 ];
    p += g_oi.rres;
    pbeyond = p + g_oi.rarg2;
    while ( p != pbeyond )
        *p++ = (uint32_t) g_oi.rtmp;
} /* memfdw_do */

#ifdef OI8

static void memfqw_do()
{
    uint64_t * p, * pbeyond;
    p = (uint64_t *) & ram[ g_oi.rarg1 ];
    p += g_oi.rres;
    pbeyond = p + g_oi.rarg2;
    while ( p != pbeyond )
        *p++ = g_oi.rtmp;
} /* memfqw_do */

#endif
#endif

static void staddb_do()
{
    uint8_t * pb, * pend, * pstart;
    oi_t tadd;
    pb = & ( ram[ g_oi.rtmp + g_oi.rarg1 ] );
    pstart = pb;
    pend = pb + ( g_oi.rres - g_oi.rtmp );
    tadd = g_oi.rarg2;

    do
    {
        *pb = 0;
        pb += tadd;
    } while ( pb <= pend );

    g_oi.rtmp += (oi_t) ( pend - pstart );
} /* staddb_do */

static void staddw_do()
{
    uint16_t * pw;
    pw = (uint16_t *) & ( ram[ g_oi.rtmp + g_oi.rarg1 ] );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        g_oi.rtmp += g_oi.rarg2;
    } while ( g_oi.rtmp <= g_oi.rres );
} /* staddw_do */

#ifndef OI2

static void stadddw_do()
{
    uint32_t * pw;
    pw = (uint32_t *) & ( ram[ g_oi.rtmp + g_oi.rarg1 ] );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        g_oi.rtmp += g_oi.rarg2;
    } while ( g_oi.rtmp <= g_oi.rres );
} /* stadddw_do */

#ifdef OI8

static void staddqw_do()
{
    uint64_t * pw;
    pw = (uint64_t *) & ( ram[ g_oi.rtmp + g_oi.rarg1 ] );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        g_oi.rtmp += g_oi.rarg2;
    } while ( g_oi.rtmp <= g_oi.rres );
} /* staddqw_do */

#endif
#endif

#ifdef OLDCPU
static void ldo_do( op ) opcode_t op;
#else
static void ldo_do( opcode_t op )
#endif
{
    size_t op1;
    oi_t val;
    uint8_t width, funct1;
    ioi_t ival;

    op1 = get_op1();
    ival = (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
    funct1 = funct_from_op( op1 );

    if ( 2 == funct1 )
        set_reg_from_op( op, (oi_t) ival );
    else /* funct1 is 1 or 2 */
    {
        if ( 1 == funct1 ) /* pre-increment register for inc variants */
            inc_reg_from_op( op1 );
    
        width = (uint8_t) width_from_op( op1 );
        val = g_oi.rpc + ival + ( ( (oi_t) 1 << width ) * get_reg_from_op( op1 ) );
    
        if ( (oi_t) 0 == width )
            set_reg_from_op( op, get_byte( val ) ); /* ldob */
        else if_1_is_width
            set_reg_from_op( op, get_word( val ) ); /* ldow */
#ifndef OI2
        else if_2_is_width
            set_reg_from_op( op, get_dword( val ) ); /* ldodw */
#ifdef OI8
        else
            set_reg_from_op( op, get_qword( val ) ); /* ldoqw */
#endif
#endif
    }
} /* ldo_do */

#ifdef OLDCPU
static void moddiv_do( op, op1 ) size_t op; size_t op1;
#else
static void moddiv_do( size_t op, size_t op1 )
#endif
{
    oi_t x, y;
    y = get_reg_from_op( op1 );
    if ( (oi_t) 0 == y )
    {
        push( 0 );
        return;
    }

    x = get_reg_from_op( op );
    set_reg_from_op( op, x % y );
    push( x / y );
} /* moddiv_do */

#ifdef OLDCPU
static oi_t frame_offset( offset ) ioi_t offset;
#else
static oi_t frame_offset( ioi_t offset )
#endif
{
    if ( offset >= 0 )
        return g_oi.rframe + ( ( 3 + offset ) * sizeof( oi_t ) );

    return g_oi.rframe + ( ( 1 + offset ) * sizeof( oi_t ) );
} /* frame_offset */

#ifdef OLDCPU
void cstf_do( op ) opcode_t op;
#else
void cstf_do( opcode_t op )
#endif
{
    oi_t val;
    uint8_t op1;

    val = get_reg_from_op( op );
    op1 = get_op1();
    if ( CheckRelation( val, get_reg_from_op( op1 ), funct_from_op( op1 ) ) )
        set_oiword( frame_offset( (ioi_t) reg_from_op( ram[ g_oi.rpc + 2 ] ) ), val );
} /* cstf_do */

#ifdef OLDCPU
bool jrelb_do( op, op1 ) opcode_t op, op1;
#else
bool jrelb_do( opcode_t op, opcode_t op1 )
#endif
{
    ioi_t ival;
    if ( CheckRelation( get_reg_from_op( op ), get_byte( get_reg_from_op( op1 ) + get_byte( g_oi.rpc + 2 ) ), funct_from_op( op1 ) ) )
    {
        ival = (ioi_t) (int8_t) get_byte( g_oi.rpc + 3 );
        if ( ival <= (ioi_t) 1 ) /* jump relative <= 1 means return. 1=nf */
        {
            pop( g_oi.rpc );
            if ( (ioi_t) 0 == ival )
                pop( g_oi.rframe );
        }
        else
            g_oi.rpc += ival;
        return true;
    }
    return false;
} /* jrelb_do */

#ifdef OLDCPU
bool jrel_do( op, op1 ) opcode_t op, op1;
#else
bool jrel_do( opcode_t op, opcode_t op1 )
#endif
{
    ioi_t ival;
    if ( CheckRelation( get_reg_from_op( op ), get_word( get_reg_from_op( op1 ) + get_byte( g_oi.rpc + 2 ) ), funct_from_op( op1 ) ) )
    {
        ival = (ioi_t) (int8_t) get_byte( g_oi.rpc + 3 );
        if ( ival <= (ioi_t) 1 ) /* jump relative <= 1 means return. 1=nf */
        {
            pop( g_oi.rpc );
            if ( (ioi_t) 0 == ival )
                pop( g_oi.rframe );
        }
        else
            g_oi.rpc += ival;
        return true;
    }
    return false;
} /* jrel_do */

#ifdef OLDCPU
void stinc_do( op ) opcode_t op;
#else
void stinc_do( opcode_t op )
#endif
{
    uint8_t width;
    int16_t val;
    oi_t inc_amount;

    val = (int16_t) get_word( g_oi.rpc + 2 );
    width = (uint8_t) width_from_op( get_op1() );
    if ( 0 == width )
        set_byte( get_reg_from_op( op ), (uint8_t) ( val & 0xff ) );
    else if_1_is_width
        set_word( get_reg_from_op( op ), (uint16_t) val );
#ifndef OI2
    else if_2_is_width
        set_dword( get_reg_from_op( op ), (uint32_t) (int32_t) val );
#ifdef OI8
    else /* 3 == width */
        set_qword( get_reg_from_op( op ), (uint64_t) (int64_t) val );
#endif
#endif

    inc_amount = (oi_t) ( 1 << width );
    add_reg_from_op( op, inc_amount );
} /* stinc_do */

#ifdef OLDCPU
void stinc_reg_do( op ) opcode_t op;
#else
void stinc_reg_do( opcode_t op )
#endif
{
    uint8_t width;
    oi_t inc_amount, val;
    opcode_t op1;

    op1 = get_op1();
    val = get_reg_from_op( op1 );
    width = (uint8_t) width_from_op( get_op1() );
    if ( 0 == width )
        set_byte( get_reg_from_op( op ), (uint8_t) val );
    else if_1_is_width
        set_word( get_reg_from_op( op ), (uint16_t) val );
#ifndef OI2
    else if_2_is_width
        set_dword( get_reg_from_op( op ), (uint32_t) val );
#ifdef OI8
    else /* 3 == width */
        set_qword( get_reg_from_op( op ), val );
#endif
#endif

    inc_amount = (oi_t) ( 1 << width );
    add_reg_from_op( op, inc_amount );
} /* stinc_reg_do */

#ifdef OLDCPU
void st_do( op ) opcode_t op;
#else
void st_do( opcode_t op )
#endif
{
    opcode_t op1, width;

    op1 = get_op1();
    width = width_from_op( op1 );
    if ( 0 == width )
        set_byte( get_reg_from_op( op ), (uint8_t) ( 0xff & get_reg_from_op( op1 ) ) );
    else if_1_is_width
        set_word( get_reg_from_op( op ), (uint16_t) get_reg_from_op( op1 ) );
#ifndef OI2
    else if_2_is_width
        set_dword( get_reg_from_op( op ), (uint32_t) get_reg_from_op( op1 ) );
#ifdef OI8
    else /* 3 == width */
        set_qword( get_reg_from_op( op ), get_reg_from_op( op1 ) );
#endif
#endif
} /* st_do */

#ifdef OLDCPU
void sto_do( op ) opcode_t op;
#else
void sto_do( opcode_t op )
#endif
{
    opcode_t op1, width;
    oi_t val;
    ioi_t ival;

    op1 = get_op1();
    ival = (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
    width = width_from_op( op1 );
    val = g_oi.rpc + ival + ( (oi_t) ( 1 << width ) * get_reg_from_op( op1 ) );
    if ( 0 == width )
        set_byte( val, (uint8_t) get_reg_from_op( op ) );
    else if_1_is_width
        set_word( val, (uint16_t) get_reg_from_op( op ) );
#ifndef OI2
    else if_2_is_width
        set_dword( val, (uint32_t) get_reg_from_op( op ) );
#ifdef OI8
    else /* 3 == width */
        set_qword( val, get_reg_from_op( op ) );
#endif
#endif
} /* sto_do */

#ifdef OLDCPU
bool op_80_90_do( op ) opcode_t op;
#else
bool op_80_90_do( opcode_t op )
#endif
{
    opcode_t op1, width;
    oi_t val;

    op1 = get_op1();
    switch( funct_from_op( op1 ) ) 
    {
        case 0: /* syscall */
        {
            val = g_oi.rpc;
            OISyscall( ( ( op << 1 ) & 0x38 ) | ( ( op1 >> 2 ) & 7 ) );
            if ( g_oi.rpc != val )
                return true;
            break;
        }
        case 1: /* pushf offset */
        {
            push( get_oiword( frame_offset( (int16_t) reg_from_op( op1 ) ) ) );
            break;
        }
        case 2: /* stst [reg0] */
        {
            pop( val );
            set_oiword( val, get_reg_from_op( op ) );
            break;
        }
        case 3: /* addimgw reg0 if width=0, subimgw if width=1, ldi if width=2 */
        {
            width = width_from_op( op1 );
            if ( 0 == width )
                set_reg_from_op( op, get_reg_from_op( op ) + (oi_t) sizeof( oi_t ) );
            else if ( 1 == width )
                set_reg_from_op( op, get_reg_from_op( op ) - (oi_t) sizeof( oi_t ) );
            break;
        }
        case 4: /* stinc [reg0], reg1  -- then increment reg0 by width of store */
        {
            stinc_reg_do( op );
            break;
        }
        case 5: /* swap reg0, reg1 */
        {
            val = get_reg_from_op( op );
            set_reg_from_op( op, get_reg_from_op( op1 ) );
            set_reg_from_op( op1, val );
            break;
        }
    }

    return false;
} /* op_80_90_do */

uint32_t ExecuteOI()
{
#ifndef OI2
    opcode_t byte_len;
#endif
    opcode_t op, op1, op2, width;
    oi_t val;
    ioi_t ival;
    uint32_t instruction_count;

    instruction_count = 0;

    do
    {
        op = get_op();
#ifndef NDEBUG
        assert( (oi_t) 0 == g_oi.rzero );
        assert( (oi_t) 0 == get_oiword( 0 ) );

        if ( g_OIState )
        {
            if ( g_OIState & OI_FLAG_TRACE_INSTRUCTIONS )
                TraceState();
        }
        instruction_count++;
#endif
        switch( op )
        {
            case 0x00: { OIHalt(); goto _all_done; } /* halt */
            case 0x04: case 0x0c: case 0x10: case 0x14: case 0x18: case 0x1c: /* inc r */
            {
                inc_reg_from_op( op );
                break;
            }
            case 0x08: /* retzero: move 0 to rres and return */
            {
                g_oi.rres = 0;
                pop( g_oi.rpc );
                pop( g_oi.rframe );
                continue;
            }
            case 0x20: /* imulst */
            {
                pop( val );
                g_oi.rres = (ioi_t) val * (ioi_t) g_oi.rres;
                break;
            }
            case 0x24: case 0x2c: case 0x30: case 0x34: case 0x38: case 0x3c: /* dec r */
            {
                dec_reg_from_op( op );
                break;
            }
            case 0x28: /* shlimg */
            {
                g_oi.rres <<= OI_IMAGE_SHIFT;
                break;
            }
            case 0x40: case 0x44: case 0x4c: case 0x50: case 0x54: case 0x58: case 0x5c: /* push r */
            {
                push( get_reg_from_op( op ) );
                break;
            }
            case 0x48: /* retzeronf */
            {
                g_oi.rres = 0;
                pop( g_oi.rpc );
                continue;
            }
            case 0x60: /* pop rzero */
            {
                pop_empty(); /* don't overwrite rzero */
                break;
            }
            case 0x64: case 0x6c: case 0x70: case 0x74: case 0x78: case 0x7c: /* pop r */
            {
                pop( val );
                set_reg_from_op( op, val );
                break;
            }
            case 0x68: /* retnf */
            {
                pop( g_oi.rpc );
                continue;
            }
            case 0x80: /* subst */
            {
                pop( val );
                g_oi.rres = val - g_oi.rres;
                break;
            }
            case 0x84: /* imgwid */
            {
                g_oi.rres = sizeof( oi_t );
                break;
            }
            case 0x8c: case 0x90: case 0x94: case 0x98: case 0x9c: /* zero r */
            {
                set_reg_from_op( op, 0 );
                break;
            }
            case 0x88: /* shrimg */
            {
                g_oi.rres >>= OI_IMAGE_SHIFT;
                break;
            }
            case 0xa0: /* addst */
            {
                pop( val );
                g_oi.rres = val + g_oi.rres;
                break;
            }
            case 0xac: case 0xb0: case 0xb4: case 0xb8: case 0xbc: /* shl r */
            {
                set_reg_from_op( op, get_reg_from_op( op ) << 1 );
                break;
            }
            case 0xa8: /* idivst */
            {
                pop( val );
                g_oi.rres = (ioi_t) val / (ioi_t) g_oi.rres;
                break;
            }
            case 0xc0: /* ret */
            {
                pop( g_oi.rpc );
                pop( g_oi.rframe );
                continue;
            }
            case 0xcc: case 0xd0: case 0xd4: case 0xd8: case 0xdc: /* shr r */
            {
                set_reg_from_op( op, get_reg_from_op( op ) >> 1 );
                break;
            }
            case 0xec: case 0xf0: case 0xf4: case 0xf8: case 0xfc: /* inv r */
            {
                set_reg_from_op( op, ! get_reg_from_op( op ) );
                break;
            }
            case 0xe0: /* andst */
            {
                pop( val );
                g_oi.rres = val & g_oi.rres;
                break;
            }
            case 0x02: case 0x06: case 0x0a: case 0x0e: /* ld r, [address] */
            case 0x12: case 0x16: case 0x1a: case 0x1e:
            {
                set_reg_from_op( op, get_oiword( get_oiword( g_oi.rpc + 1 ) ) );
                break;
            }
            case 0x22: case 0x26: case 0x2a: case 0x2e: /* ldi r, value */
            case 0x32: case 0x36: case 0x3a: case 0x3e:
            {
                set_reg_from_op( op, get_oiword( g_oi.rpc + 1 ) );
                break;
            }
            case 0x42: case 0x46: case 0x4a: case 0x4e: /* st [address], r */
            case 0x52: case 0x56: case 0x5a: case 0x5e:
            {
                set_oiword( get_oiword( g_oi.rpc + 1 ), get_reg_from_op( op ) );
                break;
            }
            case 0x62: case 0x66: case 0x6a: case 0x6e: /* jmp address */
            case 0x72: case 0x76: case 0x7a: case 0x7e:
            {
                g_oi.rpc = get_oiword( g_oi.rpc + 1 ) + ( sizeof( oi_t ) * get_reg_from_op( op ) );
                continue;
            }
            case 0x82: case 0x86: case 0x8a: case 0x8e: /* inc [address] */
            case 0x92: case 0x96: case 0x9a: case 0x9e:
            {
                ( * (oi_t *) ( ram + get_oiword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) )++;
                break;
            }
            case 0xa2: case 0xa6: case 0xaa: case 0xae: /* dec [address] */
            case 0xb2: case 0xb6: case 0xba: case 0xbe: 
            {
                ( * (oi_t *) ( ram + get_oiword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) )--;
                break;
            }
            case 0xc2: case 0xc6: case 0xca: case 0xce: /* ldae rres (implied), address[ r ] */
            case 0xd2: case 0xd6: case 0xda: case 0xde: 
            {
                g_oi.rres = get_oiword( get_oiword( g_oi.rpc + 1 ) + ( sizeof( oi_t ) * get_reg_from_op( op ) ) );
                break;
            }
            case 0xe2: case 0xe6: case 0xea: case 0xee: /* call address */
            case 0xf2: case 0xf6: case 0xfa: case 0xfe: 
            {
                push( g_oi.rframe );
                push( g_oi.rpc + 1 + sizeof( oi_t ) );
                g_oi.rframe = g_oi.rsp - sizeof( oi_t ); /* point at first local variable (if any) */
                g_oi.rpc = get_oiword( g_oi.rpc + 1 ) + ( sizeof( oi_t ) * get_reg_from_op( op ) );
                continue;
            }
            case 0x03: case 0x07: case 0x0b: case 0x0f: /* j / ji / jrelb / jrel */
            case 0x13: case 0x17: case 0x1b: case 0x1f:
            {
                op1 = get_op1();
                switch( width_from_op( op1 ) )
                {
                    case 0: /* j rleft, rright, relation, address. always native bit width */
                    {
                        if ( CheckRelation( get_reg_from_op( op ), get_reg_from_op( op1 ), funct_from_op( op1 ) ) )
                        {
                            g_oi.rpc += (int16_t) get_word( g_oi.rpc + 2 );
                            continue;
                        }
                        break;
                    }
                    case 1: /* ji rleft, rrightCONSTANT, relation, address. always native bit width. address extended to native width */
                    {
                        if ( CheckRelation( get_reg_from_op( op ), 1 + reg_from_op( op1 ), funct_from_op( op1 ) ) )
                        {
                            g_oi.rpc += (int16_t) get_word( g_oi.rpc + 2 );
                            continue;
                        }
                        break;
                    }
                    case 2: /* jrelb r0left, r1rightADDRESS, offset (from r1right), RELATION, (-128..127 pc offset) */
                    {
                        if ( jrelb_do( op, op1 ) )
                            continue;
                        break;
                    }
                    case 3: /* jrel r0left, r1rightADDRESS, offset (from r1right), RELATION, (-128..127 pc offset) */
                    {
                        if ( jrel_do( op, op1 ) )
                            continue;
                        break;
                    }
                }
                break;
            }
            case 0x23: case 0x27: case 0x2b: case 0x2f: /* stinc */
            case 0x33: case 0x37: case 0x3b: case 0x3f:
            {
                stinc_do( op );
                break;
            }
            case 0x43: case 0x47: case 0x4b: case 0x4f: /* ldinc reg0dst reg1offinc pc-relative-offset */
            case 0x53: case 0x57: case 0x5b: case 0x5f:
            {
                op1 = get_op1();
                val = get_reg_from_op( op1 ) + g_oi.rpc + (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
                width = width_from_op( op1 );
                if ( 0 == width )
                    set_reg_from_op( op, get_byte( val ) );
                else if_1_is_width
                    set_reg_from_op( op, get_word( val ) );
#ifndef OI2
                else if_2_is_width
                    set_reg_from_op( op, get_dword( val ) );
#ifdef OI8
                else /* 3 == width */
                    set_reg_from_op( op, get_qword( val ) );
#endif
#endif
                val = (oi_t) ( 1 << width );
                add_reg_from_op( op1, val );
                break;
            }
            case 0x63: case 0x67: case 0x6b: case 0x6f: /* call through function pointer table and callnf variants */
            case 0x73: case 0x77: case 0x7b: case 0x7f:
            {
                op1 = get_op1();
                ival = (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
                switch( funct_from_op( op1 ) )
                {
                    case 0: /* call address[ r0 ] */
                    {
                        push( g_oi.rframe );
                        push( g_oi.rpc + 4 );
                        g_oi.rframe = g_oi.rsp - sizeof( oi_t ); /* point at first local variable (if any) */
                        g_oi.rpc = get_oiword( g_oi.rpc + ival + ( sizeof( oi_t ) * get_reg_from_op( op ) ) );
                        continue;
                    }
                    case 1: /* callnf address[ r0 ] */
                    {
                        push( g_oi.rpc + 4 );
                        g_oi.rpc = get_oiword( g_oi.rpc + ival + ( sizeof( oi_t ) * get_reg_from_op( op ) ) );
                        continue;
                    }
                    case 2: /* callnf address */
                    {
                        push( g_oi.rpc + 4 );
                        g_oi.rpc = g_oi.rpc + ival + ( sizeof( oi_t ) * get_reg_from_op( op ) );
                        continue;
                    }
                }
            }
            case 0x83: case 0x87: case 0x8b: case 0x8f: /* sto address[ r1 ], r0 -- address is signed and pc-relative */
            case 0x93: case 0x97: case 0x9b: case 0x9f:
            {
                sto_do( op );
                break;
            }
            case 0xa3: case 0xa7: case 0xab: case 0xaf: /* ldo / ldob / ldoinc / ldoincb / ldi */
            case 0xb3: case 0xb7: case 0xbb: case 0xbf:
            {
                ldo_do( op );
                break;
            }
            case 0xc3: case 0xc7: case 0xcb: case 0xcf: /* ld / ldb / sti / stib / math */
            case 0xd3: case 0xd7: case 0xdb: case 0xdf:
            {
                op1 = get_op1();
                switch( funct_from_op( op1 ) )
                {
                    case 0:  /* ld / ldb rdst, [address] */
                    {
                        val = g_oi.rpc + (int16_t) get_word( g_oi.rpc + 2 );
                        width = width_from_op( op1 );
                        if ( 0 == width )
                            set_reg_from_op( op, get_byte( val ) );
                        else if_1_is_width
                            set_reg_from_op( op, get_word( val ) );
#ifndef OI2
                        else if_2_is_width
                            set_reg_from_op( op, get_dword( val ) );
#ifdef OI8
                        else /* 3 == width */
                            set_reg_from_op( op, get_qword( val ) );
#endif
#endif
                        break;
                    }
                    case 1: /* sti / stib [address] constant -8..7 stored in r1 */
                    {
                        val = g_oi.rpc + (int16_t) get_word( g_oi.rpc + 2 );

                        /* r0 has high 3 bits and r1 has low 3 bits of CONSTANT */
                        ival = sign_extend_oi( ( ( op << 1 ) & 0x38 ) | reg_from_op( op1 ), 5 );
                        width = width_from_op( op1 );
                        if ( 0 == width )
                            set_byte( val, (uint8_t) ival );
                        else if_1_is_width
                            set_word( val, (uint16_t) ival );
#ifndef OI2
                        else if_2_is_width
                            set_dword( val, (uint32_t) ival );
#ifdef OI8
                        else /* 3 == width */
                            set_qword( val, (uint64_t) ival );
#endif
#endif
                        break;
                    }
                    case 2: /* math r0dst, r1left, r2right, funct2MATH */
                    {
                        op2 = get_op2();
                        set_reg_from_op( op, Math( get_reg_from_op( op1 ), get_reg_from_op( op2 ), funct_from_op( op2 ) ) );
                        break;
                    }
                }
                break;
            }
            case 0xe3: case 0xe7: case 0xeb: case 0xef: /* cstf r0left, r1right, funct1REL, reg2FRAMEOFFSET */
            case 0xf3: case 0xf7: case 0xfb: case 0xff: /* fourth byte is currently unused */
            {
                cstf_do( op );
                break;
            }
            case 0x01: case 0x05: case 0x09: case 0x0d: /* math rdst/rleft, rright */
            case 0x11: case 0x15: case 0x19: case 0x1d:
            {
                op1 = get_op1();
                set_reg_from_op( op, Math( get_reg_from_op( op ), get_reg_from_op( op1 ), funct_from_op( op1 ) ) );
                break;
            }
            case 0x21: case 0x25: case 0x29: case 0x2d: /* mov r0dst, r1src / cmov r0dst, r1src, funct1REL */
            case 0x31: case 0x35: case 0x39: case 0x3d:
            {
                op1 = get_op1();
                if ( ( (uint8_t) 3 == funct_from_op( op1 ) ) || /* shortcut for NE */
                     ( CheckRelation( get_reg_from_op( op ), get_reg_from_op( op1 ), funct_from_op( op1 ) ) ) )
                    set_reg_from_op( op, get_reg_from_op( op1 ) );
                break;
            }
            case 0x41: case 0x45: case 0x49: case 0x4d: /* cmpst rdst, rright, relation */
            case 0x51: case 0x55: case 0x59: case 0x5d:
            {
                /* set rdst to boolean of ( pop() RELATION rright ) */
                op1 = get_op1();
                pop( val );
                set_reg_from_op( op, CheckRelation( val, get_reg_from_op( op1 ), funct_from_op( op1 ) ) );
                break;
            }
            case 0x61: case 0x65: case 0x69: case 0x6d: /* ldf / stf / ret x / ldib / signex / memf / stadd / moddiv */
            case 0x71: case 0x75: case 0x79: case 0x7d:
            {
                op1 = get_op1();
                switch( funct_from_op( op1 ) )
                {
                    case 0: /* ldf rdst, offset ( -4..3 ) */
                    {
                        set_reg_from_op( op, get_oiword( frame_offset( ( (int16_t) reg_from_op( op1 ) ) ) ) );
                        break;
                    }
                    case 1: /* stf rdst, offset ( -4..3 ) */
                    {
                        set_oiword( frame_offset( (int16_t) reg_from_op( op1 ) ), get_reg_from_op( op ) );
                        break;
                    }
                    case 2: /* ret x */
                    {
                        pop( g_oi.rpc );
                        pop( g_oi.rframe );
                        val = ( 1 + reg_from_op( op1 ) );
                        g_oi.rsp += ( sizeof( oi_t ) * val );
                        continue;
                    }
                    case 3: /* ldib rdst x */
                    {
                        set_reg_from_op( op, sign_extend_oi( op1 & 0x1f, 4 ) );
                        break;
                    }
                    case 4: /* signex */
                    {
                        width = width_from_op( op1 );
                        if ( 0 == width )
#ifdef AZTECCPM
                            set_reg_from_op( op, sign_extend_oi( get_reg_from_op( op ), 7 ) ); /* the casts below don't work with Aztec C */
#else
                            set_reg_from_op( op, (oi_t) (ioi_t) (int8_t) get_reg_from_op( op ) );
#endif
                        else if_1_is_width
                            set_reg_from_op( op, (oi_t) (ioi_t) (int16_t) get_reg_from_op( op ) );
#ifndef OI2
                        else if_2_is_width
                            set_reg_from_op( op, (oi_t) (ioi_t) (int32_t) get_reg_from_op( op ) );
#endif
                        break;
                    }
                    case 5: /* memf: memfill address in rarg1 with rtmp for rarg2 iterations (bytes or words ) */
                    {
                        width = width_from_op( op1 );
                        if ( 0 == width )
                            memfb_do();
                        else if_1_is_width
                            memfw_do();
#ifndef OI2
                        else if_2_is_width
                            memfdw_do();
#ifdef OI8
                        else /* 3 == width */
                            memfqw_do();
#endif
#endif
                        break;
                    }
                    case 6: /* stadd: stb [ rtmp + rarg1 ] = 0. add rtmp, rarg2. loop if rtmp le rres */
                    {
                        width = width_from_op( op1 );
                        if ( 0 == width )
                            staddb_do();
                        else if_1_is_width
                            staddw_do();
#ifndef OI2
                        else if_2_is_width
                            stadddw_do();
#ifdef OI8
                        else /* 3 == width */
                            staddqw_do();
#endif
#endif
                        break;
                    }
                    case 7: /* moddiv is frame 7. push( r0 / r1 ). r0 = r0 % r1. */
                    {
                        moddiv_do( op, op1 );
                        break;
                    }
                }
                break;
            }
            case 0x81: case 0x85: case 0x89: case 0x8d: /* syscall, pushf, stst, addimgw, subimgw */
            case 0x91: case 0x95: case 0x99: case 0x9d:
            {
                if ( op_80_90_do( op ) )
                    continue;
                break;
            }
            case 0xa1: case 0xa5: case 0xa9: case 0xad: /* st / stb [r0dst] r1src */
            case 0xb1: case 0xb5: case 0xb9: case 0xbd:
            {
                st_do( op );
                break;
            }
            case 0xc1: case 0xc5: case 0xc9: case 0xcd: /* ld r0dst [r1src] */
            case 0xd1: case 0xd5: case 0xd9: case 0xdd:
            {
                op1 = get_op1();
                width = width_from_op( op1 );
                if ( 0 == width )
                    set_reg_from_op( op, get_byte( get_reg_from_op( op1 ) ) );
                else if_1_is_width
                    set_reg_from_op( op, get_word( get_reg_from_op( op1 ) ) );
#ifndef OI2
                else if_2_is_width
                    set_reg_from_op( op, get_dword( get_reg_from_op( op1 ) ) );
#ifdef OI8
                else /* 3 == width */
                    set_reg_from_op( op, get_qword( get_reg_from_op( op1 ) ) );
#endif
#endif
                break;
            }
            case 0xe1: case 0xe5: case 0xe9: case 0xed: /* mathst r0dst, r1src, Math */
            case 0xf1: case 0xf5: case 0xf9: case 0xfd:
            {
                op1 = get_op1();
                pop( val );
                val = Math( val, get_reg_from_op( op1 ), funct_from_op( op1 ) );
                set_reg_from_op( op, val );
                break;
            }
            default:
                illegal_instruction( op, get_op1() );
        }

#ifdef OI2
        g_oi.rpc += ( 1 + byte_len_from_op( op ) );
#else
        byte_len = 1 + byte_len_from_op( op );
        if ( 3 == byte_len )
            byte_len += ( sizeof( oi_t ) - 2 );
        g_oi.rpc += (oi_t) byte_len;
#endif

        continue; /* old compilers otherwise evaluate (true) each loop */
    } while ( true );

_all_done:
    return instruction_count;
} /* ExecuteOI */

