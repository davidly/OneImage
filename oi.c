/*
    OneImage bytecode interpreter.
    Written by David Lee in August 2024.

    The OneImage instruction set is designed for:
        -- efficiency of power consumption, execution time, and RAM usage
        -- ease of targeting for compilers
        -- support a single assembler source file across 16, 32, and 64 bit execution environments
        -- taking the best instructions from many different ISAs and bytecode interpreters
        -- introducing new instructions that are simple yet helpful for typical apps

    Native Width: number of bytes for registers, pointers, etc. 16, 32, or 64. Build with defines OI2, OI4, or OI8
        This is the width in hardware or in an emulator.

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
        3 bit IDs 0..7 for:    gt, lt, eq, ne, ge, le, even, odd
        even is true if the lhs is even. the rhs is ignored
        odd is true if the lhs is odd. the rhs is ignored

    Operation: Another name for Function when it's not Math or Relation

    Instruction value Width: This can be <= image width, which can be <= native width.
        2 lower bits 1..0 of byte 1 of 2 and 4 byte operations. 0 = byte, 1 = word, 2 = dword, 3 = qword
        when width doesn't apply to an instruction, this field can instead differentiate between instructions

    Opcode lengths:
        4 byte operations:
                    7..5        4..2            1..0
            byte 0: operation   Registers lhs   3
            byte 1: Relations   Registers rhs   Width
            bytes 2/3: 16 bit value
            note: all addresses are pc-relative signed 16-bit values
            funct0:
                0:  j / ji / jrel / jrelb   j if width = 0
                                            ji if width = 1
                                            jrelb if width = 2
                                            jrel if width = 3
                                       j r0left, r1right, RELATION, ADDRESS
                                       ji r0left, r1rightIMMEDIATE, RELATION, ADDRESS.
                                           r1rightIMMEDIATE is unsigned value + 1. == 1..8
                                       jrel r0left, r1rightADDRESS, offset (from r1right), RELATION (-128..127 pc offset)
                                           for jrel, the address offset is unsigned. the pc offset is signed.
                                           jrelb checks byte values. jrel checks native width values
                                       pc offset special cases:
                                           0: ret
                                           1: retnf
                                           2: ret0
                                           3: ret0nf
                1:  stinc / stincb:    [ r0off ] = value; inc r0off per width
                2:  ldinc / ldincb:    r0dst = address[ r1off ]. inc r1off per width.
                    exceptions:          overridden
                        43 UNUSED --     ldinc rzero, ...
                3:  value of funct in op1:
                    0:   call:         functiontableaddress[ r0 ]
                    1:   callnf:       functiontableaddress[ r0 ]   (don't push or setup rframe).
                    2:   callnf:       address + r0   (don't push or setup rframe).
                4:  sto / stob:        address[ r1offset ] = r0value. offset is multiplied by width
                5:  value of funct in op1
                    0:   ldo:          r0dst = address[ r1off ]. offset is multiplied by width
                    1:   ldoinc:       r1++ (always one independent of width) then r0dst = address[ r1off ]. offset is multiplied by width
                    2:   ldiw          constant -32768..32767 sign extended. use ldb if possible and ldi's 3-byte form if the number is large or a 2-byte native width
                    exceptions:          overridden
                        a3 cpuinfo --     ldo/ldi rzero, ... / returns rres_16 version, rtmp_16 2 ascii char ID
                6:  value of funct in op1
                    0:   ld:           r0dst = [ address ]
                    1:   sti:          [address], r1 -8..7
                    2:   math r0dst, r1src, r2src, f2Math
                    3:   cmp r0dst, r1src, r2src, f2Relation
                    4:   fzero r0index, r1address, 2-byte unsigned max count of width items. 0..65534
                             on return, r0offset is an index to an array entry with a 0 value or 1 + count
                    5:   stoi r0address[ r1index ], 2-byte sign-extended constant
                    6:   stor r0address[ r1index ], r2value
                    7:   ldor r0destination, r1address[ r3index ]
                7:  cstf               conditional stack frame store: cstf r0left r1right frame1REL reg2FRAMEOFFSET

        3 byte operations: high 3 bits 0..7:  ld, ldi, st, jmp, inc, dec, ldae, call
            ldae target is always rres. register is multiplied by image width and added to address for read. load array entry.
            following the first byte are image width bytes: 2, 4, 8
            jmp, inc, dec, call all add unsigned reg0 to target address
            3-byte instructions always opererate on image width quantities
            exceptions:      overridden
               02 UNUSED  -- ld rzero, [address]
               22 UNUSED  -- ldi rzero, XXXX

        2 byte operations:
                    7..5                 4..2            1..0
            byte 0: Operation            r0 lhs          01
                0: Math r0dst r1src
                1: cmov r0dst, r1src, Relation. if ( r0dst Relation r1src ) r0dst = rssrc.
                   mov r0dst, r1src     -- this maps to cmov r0dst, r1src, ne
                   exceptions:      overridden
                     21 UNUSED     -- cmov rzero, ...
                2: cmpst r0dst, r1src, Relation -- r0dst = ( pop() Relation r1src ). sets r0dst to a boolean 1 or 0
                3: - 0/1 funct: ldf/stf. loads and stores r0 relative to rframe.
                                r1 >= 0 is is frame[ ( 3 + r1 ) * sizeof( oi_t ) ]. r1 < 0 is frame[ ( 1 + r1 ) * sizeof( oi_t ) ]
                   - 2 funct: ret x     -- pop x items off the stack and return
                   - 3 funct: ldib x    -- load immediate small signed values
                   - 4 funct: signex    -- sign extend the specified width to the native width
                   - 5 funct: memf. rarg1 = address, rarg2 = # of items to copy. rtmp = value to copy. -- memfill
                   - 6 funct: stadd. stb [rtmp+rarg1] = 0. add rtmp, rarg2. loop if rtmp le rarg1 -- store and add in a loop. rtmp is undefined after execution
                   - 7 funct: moddiv. r0 = r0 % r1. push( r0 / r1 ). -- calculate both mod and div
                4:
                   - 0 funct: syscall ( ( reg of byte 0 << 3 ) | ( reg of byte 1 ) ). 6 bit system ID. bit width must be 0.
                   - 1 funct: pushf reg1CONSTANT. r1 >= 0 is is frame[ ( 3 + r1 ) * sizeof( oi_t ) ]. r1 < 0 is frame[ ( 1 + r1 ) * sizeof( oi_t ) ]
                   - 2 funct: stst reg0.  i.e.  st [pop()], reg0
                   - 3 funct: width 0: addimgw reg0      adds the image's byte width
                              width 1: subimgw reg0
                   - 4 funct: stinc [reg0], reg1 then increment reg0 by width of store
                   - 5 funct: swap reg0, reg1
                   - 6 funct: width 0: addnatw reg0      adds the hardware's native byte width
                              width 1: subnatw reg0
                5:
                   - 0 funct: st [r0dst], r1src 
                   - 1 funct: ld r0dst, [r1src]
                   - 2 funct: pushtwo  r0, r1
                   - 3 funct: poptwo   r0, r1
                6: mov r0dst, r1src.   unconditional. in addition to cmov for faster perf
                   exceptions:      overridden
                     c1 UNUSED     -- mov rzero, ...
                7: mathst r0dst, r1src, Math   -- r0dst = ( pop() MATH r1src )
            byte 1: Math/Relation/Funct  r1 rhs          bit width ( 0=8, 1=16...)

        1 byte operations: high 3 bits 0..7 operate on reg0:   inc, dec, push, pop, zero, shl, shr, inv
              exceptions:      overridden
              00 halt       -- inc rzero
              08 ret0       -- inc rsp
              20 imulst     -- dec rzero
              28 shlimg     -- dec rsp. shift rres left based on image width 2=>1, 4=>2, 8=>3
              40 push rzero -- DON'T OVERRIDE (idivst)
              48 ret0nf     -- push rsp (nf = no rframe restoration)
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
              c8 natwid     -- shr rsp
              e0 andst      -- inv rzero
              e4 UNUSED     -- inv rpc
              e8 UNUSED     -- inv rsp

    Note on coding conventions and engineering decisions:
      -- this is in C, not C++ so compilers work, including Aztec C from 1984 for 8080 CP/M 2.2.
      -- there are extra, unnecessary casts to appease older compilers.
      -- no #elif or defined() exists on older compilers.
      -- older compilers require #ifdef and #define statements to start in the first column
      -- older compilers require variable declarations prior to any other type of statement in a function
      -- older compilers don't support initializing variables when they are declared
      -- in some cases assignments to variables are made to reduce later expression complexity so older compilers work
      -- generally variables aren't used to reduce expression complexity because older compilers generate much
         slower code to save and load variables. This means code is duplicated, but if that bloat can be tolerated
         then code on old machines runs much faster.
      -- Aztec C has bugs with some casts, so there are workarounds
      -- some type names start with t_ instead of ending with _t due to symbol name length limitations in older compilers
      -- inlining of functions doesn't work on older CP/M and DOS compilers, so:
              -- Portions that need to be inlined are in #define rather than functions, even though that's bad engineering
              -- Microsoft C v6 for DOS will only optimize functions smaller than some size, so ExecuteOI() calls helper
                 functions instead of just putting the code inline. Functions calls are slower, but without optimizations
                 the emulator runs 2x slower on DOS. Less-frequently-used instructions are generally out of line.
              -- Modern compilers will inline everything anyway, so there is no performance cost of helper functions
      -- older compilers require a different function declaration syntax
      -- stack allocation and alignment is always native width even when running more narrow image width binaries.
              -- This greatly simplifies both the emulator and any potential physical implementation
              -- Code that references the stack must be aware of this and use natwid instead of imgwid for offset calculations

    Tested compilers:
        - CP/M 2.2 on 8080 and Z80:
              - HiSoft HI-TECH C v3.09   ===> this one is more robust and produces faster code
              - MANX Aztec C v1.06D      ===> this one works OK
        - DOS 3.x on 8086:
              - Microsoft C v6.00AX      ===> this one produces the fastest code by far
              - Open Watcom C/C++ x86 16-bit Compile and Link Utility Version 2.0 beta Oct  9 2023 02:19:55 (64-bit)
        - Windows 11 on x64:
              - Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33814 for x64
              - Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33814 for x86
              - g++ (Rev3, Built by MSYS2 project) 13.2.0
        - Ubuntu 22.04.3 on x64
              - g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
        - Ubuntu 22.04.2 on Arm64
              - g++ (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
        - Debian GNU/Linux 12 on RISC-V 64 bit
              - g++ (Debian 13.2.0)
        - Debian GNU/Linux 10 on Raspberry PI 4 Arm32
              - g++ (Debian 8.3.0)
        - Debian GNU/Linux 12 on Raspberry PI 5 Arm64
              - g++ (Debian 12.2.0-14)
        - MacOS Sonoma 14.6.1 (23G93) on MacBookAir M3 Arm64
              - Apple clang version 15.0.0 (clang-1500.3.9.4)

    Build notes:
        - One of OI2, OI4, or OI8 must be defined when compiling
        - The resulting oios executable can only run .oi binaries with image-width <= native width (OIx)
        - With the 8-bit compilers I can only get OI2 builds to work
        - With the 16-bit compilers only OI2 and OI4 work.
        - With 32-bit and 64-bit compilers all widths work.
*/

#include <stdio.h>

#ifdef AZTECCPM
#define memset( p, val, len ) setmem( p, len, val )
#else
#include <assert.h>
#include <string.h>
#ifndef MSC6
#include <stdint.h>
#endif /* MSC6 */
#endif /* AZTECCPM */

#include "oi.h"
#include "trace.h"

#define true 1
#define false 0

#define OI_FLAG_TRACE_INSTRUCTIONS 1

struct OneImage g_oi;

#ifdef OLDCPU /* CP/M machines with 64k or less total ram */
static uint8_t ram[ 32767 ];
#else
#ifdef MSC6 /* DOS version is built with small memory model */
static uint8_t ram[ 60000 ];
#endif /* MSC6 */
#ifdef WATCOM
static uint8_t ram[ 60000 ];
#else /* WATCOM */
static uint8_t ram[ 8 * 1024 * 1024 ]; /* arbitrary */
#endif /* WATCOM */
#endif /* OLDCPU */

#ifndef NDEBUG
static uint8_t g_OIState = 0;
#ifdef OLDCPU
void TraceInstructionsOI( t ) bool t;
#else
void TraceInstructionsOI( bool t )
#endif /* OLDCPU */
{ if ( t ) g_OIState |= OI_FLAG_TRACE_INSTRUCTIONS; else g_OIState &= ~OI_FLAG_TRACE_INSTRUCTIONS; }
#endif /* NDEBUG */

/* this choice is for performance on various platforms */

#ifdef OLDCPU
    typedef uint8_t opcode_t;
#else
    typedef size_t opcode_t;
#endif /* OLDCPU */

/* these macros lose safety and readability over inlined functions, but older compilers can't inline functions */

#ifdef OI2
#define access_ram( address ) ( ram[ address ] )
#define ram_address( address ) ( ram + ( address ) )
#define IMAGE_WIDTH 2
#else
#define access_ram( address ) ( ram[ ( address ) & g_oi.address_mask ] )
#define ram_address( address ) ( & ram[ ( address ) & g_oi.address_mask ] )
#define IMAGE_WIDTH g_oi.image_width
#endif

#define get_byte( address ) ( access_ram( address ) )
#define set_byte( address, val ) ( access_ram( address ) = val )

#define get_word( address ) ( * (uint16_t *) ( ram_address( address ) ) )
#define set_word( address, val ) ( * (uint16_t *) ( ram_address( address ) ) = val )

#define get_oiword( address ) ( * (oi_t *) ( ram_address( address ) ) )
#define set_oiword( address, val ) ( * (oi_t *) ( ram_address( address ) ) = val )

#ifdef OI4
/* no mask required */
#define get_dword( address ) ( * (uint32_t *) ( ram + address ) )
#define set_dword( address, val ) ( * (uint32_t *) ( ram + address ) = val )
#else
#define get_dword( address ) ( * (uint32_t *) ( ram_address( address ) ) )
#define set_dword( address, val ) ( * (uint32_t *) ( ram_address( address ) ) = val )
#endif /* OI4 */

#ifdef OI8
/* no mask required */
#define get_qword( address ) ( * (uint64_t *) ( ram + address ) )
#define set_qword( address, val ) ( * (uint64_t *) ( ram + address ) = val )
#endif /* OI8 */

#ifdef OI8
#define get_preg_from_op( op ) ( (oi_t *) ( ( (uint8_t *) & g_oi.rzero ) + ( ( op << 1 ) & 0x38 ) ) )
#else
#define get_preg_from_op( op ) ( (oi_t *) ( ( (uint8_t *) & g_oi.rzero ) + ( op & 0x1c ) ) )
#endif /* OI8 */

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

#ifdef OLDCPU
typedef oi_t t_pget_imgword();
typedef void t_pset_imgword();
#else
typedef oi_t t_pget_imgword( oi_t address );
typedef void t_pset_imgword( oi_t address, oi_t value );
#endif

#ifdef OLDCPU
oi_t get_imgword( address ) oi_t address;
#else
oi_t get_imgword( oi_t address )
#endif
{
    return (ioi_t) (int16_t) get_word( address );
} /* get_imgword */

#ifndef OI2
oi_t get_imgdword( oi_t address )
{
    return (ioi_t) (int32_t) get_dword( address );
} /* get_imgdword */
#endif /* OI2 */

#ifdef OI8
oi_t get_imgqword( oi_t address )
{
    return (oi_t) get_qword( address );
} /* get_imgqword */
#endif /* OI8 */

#ifdef OLDCPU
void set_imgword( address, value ) oi_t address; oi_t value;
#else
void set_imgword( oi_t address, oi_t value )
#endif
{
    set_word( address, (uint16_t) value );
} /* set_imgword */

#ifndef OI2
void set_imgdword( oi_t address, oi_t value )
{
    set_dword( address, (uint32_t) value );
} /* set_imgdword */
#endif /* OI2 */

#ifdef OI8
void set_imgqword( oi_t address, oi_t value )
{
    set_qword( address, value );
} /* set_imgqword */
#endif /* OI8 */

t_pget_imgword * pget_imgword;
t_pset_imgword * pset_imgword;

#ifdef OI2
#define if_1_is_width
#define read_imgword get_word
#define write_imgword set_word
#else
#define if_1_is_width if ( 1 == width )
#define read_imgword (*pget_imgword)
#define write_imgword (*pset_imgword)
#endif /* OI2 */

#ifdef OI4
#define if_2_is_width
#else
#define if_2_is_width if ( 2 == width )
#endif /* OI4 */

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
    g_oi.image_width = imageWidth; /* 8, 4, or 2. byte length of addresses, operands, etc. for the executable file */
    g_oi.three_byte_len = (uint8_t) 1 + imageWidth;

    if ( 2 == imageWidth )
    {
        g_oi.image_shift = 1;
#ifdef OI2
        pget_imgword = 0; /* shouldn't be called */
        pset_imgword = 0; /* shouldn't be called */
#else
        pget_imgword = get_imgword;
        pset_imgword = set_imgword;
        g_oi.address_mask = 0xffff;
#endif /* OI2 */

    }
#ifndef OI2
    else if ( 4 == imageWidth )
    {
        g_oi.image_shift = 2;
        pget_imgword = get_imgdword;
        pset_imgword = set_imgdword;
        g_oi.address_mask = 0xffffffff;
    }
#endif /* OI2 */
#ifdef OI8
    else if ( 8 == imageWidth )
    {
        g_oi.image_shift = 3;
        pget_imgword = get_imgqword;
        pset_imgword = set_imgqword;
        g_oi.address_mask = 0xffffffffffffffff;
    }
#endif /* OI8 */

    push( 0 );  /* rframe */
    push( 0 );  /* return address is 0, which has a halt instruction */
    g_oi.rframe = g_oi.rsp - sizeof( oi_t ); /* point frame at first local variable (if any) */
} /* ResetOI */

#ifdef OLDCPU
uint32_t RamInformationOI( required, ppRam, imageWidth ) uint32_t required; uint8_t ** ppRam; uint8_t imageWidth;
#else
uint32_t RamInformationOI( uint32_t required, uint8_t ** ppRam, uint8_t imageWidth )
#endif
{
    uint32_t available;

    available = (uint32_t) sizeof( ram );
    if ( ( 2 == imageWidth ) && ( available > 65536 ) )
        available = 65536;

    if ( available >= required )
        *ppRam = ram;
    else
        *ppRam = 0;
    return available;
} /* RamInformationOI */

#ifdef OLDCPU
static bool CheckRelation( l, r, relation ) ioi_t l; ioi_t r; uint8_t relation;
#else
static bool CheckRelation( ioi_t l, ioi_t r, uint8_t relation )
#endif
{
    __assume( relation <= 7 );

#ifdef OI2
    switch ( relation )
    {
        case 0: { return ( l > r ); }
        case 1: { return ( l < r ); }
        case 2: { return ( l == r ); }
        case 3: { return ( l != r ); }
        case 4: { return ( l >= r ); }
        case 5: { return ( l <= r ); }
        case 6: { return ( (bool) !( l & (ioi_t) 1 ) ); }
        case 7: { return ( (bool) ( l & (ioi_t) 1 ) ); }
        default: { __assume( false ); }
    }
#else /* OI2 */
    if ( 2 == g_oi.image_width )
    {
        switch ( relation )
        {
            case 0: { return ( (int16_t) l > (int16_t) r ); }
            case 1: { return ( (int16_t) l < (int16_t) r ); }
            case 2: { return ( (int16_t) l == (int16_t) r ); }
            case 3: { return ( (int16_t) l != (int16_t) r ); }
            case 4: { return ( (int16_t) l >= (int16_t) r ); }
            case 5: { return ( (int16_t) l <= (int16_t) r ); }
            case 6: { return ( (bool) !( l & (ioi_t) 1 ) ); }
            case 7: { return ( (bool) ( l & (ioi_t) 1 ) ); }
            default: { __assume( false ); }
        }
    }
    else if ( 4 == g_oi.image_width )
    {
        switch ( relation )
        {
            case 0: { return ( (int32_t) l > (int32_t) r ); }
            case 1: { return ( (int32_t) l < (int32_t) r ); }
            case 2: { return ( (int32_t) l == (int32_t) r ); }
            case 3: { return ( (int32_t) l != (int32_t) r ); }
            case 4: { return ( (int32_t) l >= (int32_t) r ); }
            case 5: { return ( (int32_t) l <= (int32_t) r ); }
            case 6: { return ( (bool) !( l & (ioi_t) 1 ) ); }
            case 7: { return ( (bool) ( l & (ioi_t) 1 ) ); }
            default: { __assume( false ); }
        }
    }
#ifdef OI8
    else if ( 8 == g_oi.image_width )
    {
        switch ( relation )
        {
            case 0: { return ( l > r ); }
            case 1: { return ( l < r ); }
            case 2: { return ( l == r ); }
            case 3: { return ( l != r ); }
            case 4: { return ( l >= r ); }
            case 5: { return ( l <= r ); }
            case 6: { return ( (bool) !( l & (ioi_t) 1 ) ); }
            case 7: { return ( (bool) ( l & (ioi_t) 1 ) ); }
            default: { __assume( false ); }
        }
    }
#endif /* OI8 */
    else
        return false;
#endif /* OI2 */

    assert( false );

#ifdef WATCOM
    return false;
#endif /* WATCOM */
} /* CheckRelation */

#ifdef OLDCPU
static oi_t Math( l, r, math ) oi_t l; oi_t r; uint8_t math;
#else
static oi_t Math( oi_t l, oi_t r, uint8_t math )
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
        case 7: { return l != r; }         /* true if !=, false if ==. ( 0 != ( left - right ) ) */
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
#endif /* OI8 */
#endif /* OI2 */

    return ac;
} /* render_value */

void TraceStateOI()
{
    trace( "%s", DisassembleOI( ram_address( g_oi.rpc ), g_oi.rpc, IMAGE_WIDTH ) );
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
    uint8_t * popcodes = ram_address( g_oi.rpc );

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

    pdis = DisassembleOI( popcodes, g_oi.rpc, g_oi.image_width );
    if ( ! *pdis )
        illegal_instruction( op, op1 );

    trace( "%s\n", pdis );
} /* TraceState */
#endif /* NDEBUG */

/* memf: rarg1 = array address, rarg2 = # of items (based on width) to fill. rtmp = value to copy. rres = first element to fill */

static void memfb_do()
{
    memset( ram_address( g_oi.rarg1 + g_oi.rres ), (uint8_t) g_oi.rtmp, (size_t) g_oi.rarg2 );
} /* memfb_do */

static void memfw_do()
{
    uint16_t * pw, * pbeyond, val;
    pw = (uint16_t *) ram_address( g_oi.rarg1 );
    pw += g_oi.rres;
    pbeyond = pw + g_oi.rarg2;
    val = (uint16_t) g_oi.rtmp;
    while ( pw != pbeyond )
        *pw++ = val;
} /* memfw_do */

#ifndef OI2

static void memfdw_do()
{
    uint32_t * p, * pbeyond, val;
    p = (uint32_t *) ram_address( g_oi.rarg1 );
    p += g_oi.rres;
    pbeyond = p + g_oi.rarg2;
    val = (uint32_t) g_oi.rtmp;
    while ( p != pbeyond )
        *p++ = val;
} /* memfdw_do */

#ifdef OI8

static void memfqw_do()
{
    uint64_t * p, * pbeyond, val;
    p = (uint64_t *) ram_address( g_oi.rarg1 );
    p += g_oi.rres;
    pbeyond = p + g_oi.rarg2;
    val = g_oi.rtmp;
    while ( p != pbeyond )
        *p++ = val;
} /* memfqw_do */

#endif /* OI8 */
#endif /* OI2 */

static void staddb_do()
{
    uint8_t * pb, * pend;
    oi_t tadd;
    pb = ram_address( g_oi.rtmp + g_oi.rarg1 );
    pend = pb + ( g_oi.rres - g_oi.rtmp );
    tadd = g_oi.rarg2;

    do
    {
        *pb = 0;
        pb += tadd;
    } while ( pb <= pend );
} /* staddb_do */

static void staddw_do()
{
    uint16_t * pw;
    oi_t cur;
    cur = g_oi.rtmp;
    pw = (uint16_t *) ram_address( ( sizeof( uint16_t ) * cur ) + g_oi.rarg1 );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        cur += g_oi.rarg2;
    } while ( cur <= g_oi.rres );
} /* staddw_do */

#ifndef OI2

static void stadddw_do()
{
    uint32_t * pw;
    oi_t cur;
    cur = g_oi.rtmp;
    pw = (uint32_t *) ram_address( ( sizeof( uint32_t ) * cur ) + g_oi.rarg1 );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        cur += g_oi.rarg2;
    } while ( cur <= g_oi.rres );
} /* stadddw_do */

#ifdef OI8

static void staddqw_do()
{
    uint64_t * pw;
    oi_t cur;
    cur = g_oi.rtmp;
    pw = (uint64_t *) ram_address( ( sizeof( uint64_t ) * cur ) + g_oi.rarg1 );
    do
    {
        *pw = 0;
        pw += g_oi.rarg2;
        cur += g_oi.rarg2;
    } while ( cur <= g_oi.rres );
} /* staddqw_do */

#endif /* OI8 */
#endif /* OI2 */

#ifdef OLDCPU
static void moddiv_do( op, op1 ) size_t op; size_t op1;
#else
__forceinline static void moddiv_do( size_t op, size_t op1 )
#endif
{
    oi_t x, y;
    y = get_reg_from_op( op1 );
    if ( (oi_t) 0 == y )
    {
        push( 0 ); /* when they exist, an exception should be raised */
        return;
    }

    x = get_reg_from_op( op );
    set_reg_from_op( op, x % y );
    push( x / y );
} /* moddiv_do */

#ifdef OLDCPU
static oi_t frame_offset( offset ) ioi_t offset;
#else
__forceinline static oi_t frame_offset( ioi_t offset )
#endif
{
    return g_oi.rframe + ( sizeof( oi_t ) * ( offset + ( ( offset >= 0 ) ? 3 : 1 ) ) );
} /* frame_offset */

#ifdef OLDCPU
void cstf_do( op ) opcode_t op;
#else
__forceinline void cstf_do( opcode_t op )
#endif
{
    oi_t val;
    uint8_t op1;

    val = get_reg_from_op( op );
    op1 = get_op1();
    if ( CheckRelation( val, get_reg_from_op( op1 ), funct_from_op( op1 ) ) )
        set_oiword( frame_offset( (ioi_t) reg_from_op( get_byte( g_oi.rpc + 2 ) ) ), val );
} /* cstf_do */

#ifdef OLDCPU
static void jump_return( ival ) ioi_t ival;
#else
__forceinline static void jump_return( ioi_t ival )
#endif
{
    /* jump relative <= 3 means return: 0=ret, 1=retnf, 2=ret0, 3=ret0nf */
    assert( ival >= 0 && ival <= 3 );
    pop( g_oi.rpc );
    if ( (ioi_t) 0 == ( ival & 1 ) )
        pop( g_oi.rframe );
    if ( ival >= (ioi_t) 2 )
        g_oi.rres = 0;
} /* jump_return */

#ifdef OLDCPU
static bool jrel_do( op, op1 ) opcode_t op, op1;
#else
__forceinline static bool jrel_do( opcode_t op, opcode_t op1 )
#endif
{
    ioi_t ival;
    if ( CheckRelation( get_reg_from_op( op ), read_imgword( get_reg_from_op( op1 ) + get_byte( g_oi.rpc + 2 ) ), funct_from_op( op1 ) ) )
    {
        ival = (ioi_t) (int8_t) get_byte( g_oi.rpc + 3 );
        if ( (oi_t) ival <= (oi_t) 3 )
            jump_return( ival );
        else
            g_oi.rpc += ival;
        return true;
    }
    return false;
} /* jrel_do */

#ifdef OLDCPU
static void stinc_do( op ) opcode_t op;
#else
__forceinline static void stinc_do( opcode_t op )
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
#endif /* OI8 */
#endif /* OI2 */

    inc_amount = (oi_t) ( 1 << width );
    add_reg_from_op( op, inc_amount );
} /* stinc_do */

#ifdef OLDCPU
static void op_a0_b0_do( op ) opcode_t op;
#else
__forceinline static void op_a0_b0_do( opcode_t op )
#endif
{
    opcode_t op1, width;
    oi_t val;
    uint8_t funct1;
    op1 = get_op1();
    width = width_from_op( op1 );
    funct1 = funct_from_op( op1 );

    switch ( funct1 )
    {
        case 0: /* st */
        {
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
#endif /* OI8 */
#endif /* OI2 */
            break;
        }
        case 1: /* ld */
        {
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
#endif /* OI8 */
#endif /* OI2 */
            break;
        }
        case 2: /* pushtwo */
        {
            push( get_reg_from_op( op ) );
            push( get_reg_from_op( op1 ) );
            break;
        }
        default: /* ( 3 == funct1 ) poptwo */
        {
            pop( val );
            set_reg_from_op( op, val );
            pop( val );
            set_reg_from_op( op1, val );
            break;
        }
    }
} /* ld_st_reg_reg_do */

#ifdef OLDCPU
static bool op_80_90_do( op ) opcode_t op;
#else
__forceinline static bool op_80_90_do( opcode_t op )
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
            write_imgword( val, get_reg_from_op( op ) );
            break;
        }
        case 3: /* addimgw reg0 if width=0, subimgw if width=1 */
        {
            width = width_from_op( op1 );
            if ( 0 == width )
                set_reg_from_op( op, get_reg_from_op( op ) + (oi_t) IMAGE_WIDTH );
            else if ( 1 == width )
                set_reg_from_op( op, get_reg_from_op( op ) - (oi_t) IMAGE_WIDTH );
            break;
        }
        case 4: /* stinc [reg0], reg1  -- then increment reg0 by width of store */
        {
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
#endif /* OI8 */
#endif /* OI2 */

            if ( width > 0 )
                val = (oi_t) ( 1 << width );
            else
                val = (oi_t) 1;
            add_reg_from_op( op, val );
            break;
        }
        case 5: /* swap reg0, reg1 */
        {
            val = get_reg_from_op( op );
            set_reg_from_op( op, get_reg_from_op( op1 ) );
            set_reg_from_op( op1, val );
            break;
        }
        case 6: /* addnatw reg0 if width=0, subnatw if width=1 */
        {
            width = width_from_op( op1 );
            if ( 0 == width )
                set_reg_from_op( op, get_reg_from_op( op ) + (oi_t) sizeof( oi_t ) );
            else if ( 1 == width )
                set_reg_from_op( op, get_reg_from_op( op ) - (oi_t) sizeof( oi_t ) );
            break;
        }
    }

    return false;
} /* op_80_90_do */

#ifdef OLDCPU
static void ldinc_do( op ) opcode_t op;
#else
__forceinline static void ldinc_do( opcode_t op )
#endif
{
    uint8_t op1, width;
    oi_t val;

    op1 = get_op1();
    val = get_reg_from_op( op1 ) + g_oi.rpc + (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
    width = (uint8_t) width_from_op( op1 );
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
#endif /* OI8 */
#endif /* OI2 */
    val = (oi_t) ( 1 << width );
    add_reg_from_op( op1, val );
} /* ldinc_do */

#ifdef OLDCPU
static void cmov_do( op ) opcode_t op;
#else
__forceinline static void cmov_do( opcode_t op )
#endif
{
    uint8_t op1;
    op1 = get_op1();
    if ( ( (uint8_t) 3 == funct_from_op( op1 ) ) || /* shortcut for NE */
         ( CheckRelation( get_reg_from_op( op ), get_reg_from_op( op1 ), funct_from_op( op1 ) ) ) )
        set_reg_from_op( op, get_reg_from_op( op1 ) );
} /* cmov_do */

#ifdef OLDCPU
static void signex_do( op ) opcode_t op;
#else
__forceinline static void signex_do( opcode_t op )
#endif
{
    opcode_t width;
    width = width_from_op( get_op1() );
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
#endif /* OI2 */
} /* signex_do */

#ifdef OLDCPU
static void op_c0_d0_do( op ) opcode_t op;
#else
__forceinline static void op_c0_d0_do( opcode_t op )
#endif
{
    opcode_t op1, op2, width;
    oi_t val;
    ioi_t ival;
    uint8_t * pb;
    uint16_t * pw, val16;
    oi_t index, limit;
#ifndef OI2
    uint32_t * pdw;
#ifdef OI8
    uint64_t * pqw;
#endif /* OI8 */
#endif /* OI2 */

    op1 = get_op1();
    switch( funct_from_op( op1 ) )
    {
        case 0:  /* ld / ldb rdst, [address] */
        {
            if ( 0 == reg_from_op( op ) ) /* can't write to rzero */
                break;

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
#endif /* OI8 */
#endif /* OI2 */
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
#endif /* OI8 */
#endif /* OI2 */
            break;
        }
        case 2: /* math r0dst, r1left, r2right, funct2MATH */
        {
            if ( 0 == reg_from_op( op ) ) /* can't write to rzero */
                break;

            op2 = get_op2();
            set_reg_from_op( op, Math( get_reg_from_op( op1 ), get_reg_from_op( op2 ), funct_from_op( op2 ) ) );
            break;
        }
        case 3: /* cmp r0dst, r1left, r2right, funct2RELATION */
        {
            if ( 0 == reg_from_op( op ) ) /* can't write to rzero */
                break;

            op2 = get_op2();
            set_reg_from_op( op, (oi_t) CheckRelation( get_reg_from_op( op1 ), get_reg_from_op( op2 ), funct_from_op( op2 ) ) );
            break;
        }
        case 4: /* fzero r0index, r1array, MAX 0..65535 */
        {
            /* while index < MAX, look for a 0 at each index in the array */
            limit = get_word( g_oi.rpc + 2 );
            index = (oi_t) get_reg_from_op( op );
            width = width_from_op( op1 );
            if ( 0 == width )
            {
                pb = ram_address( get_reg_from_op( op1 ) );
                while ( ( index < limit ) && ( 0 != pb[ index ] ) )
                    index++;
            }
            else if_1_is_width
            {
                pw = (uint16_t *) ram_address( get_reg_from_op( op1 ) );
                while ( ( index < limit ) && ( 0 != pw[ index ] ) )
                    index++;
            }
#ifndef OI2
            else if_2_is_width
            {
                pdw = (uint32_t *) ram_address( get_reg_from_op( op1 ) );
                while ( ( index < limit ) && ( (uint32_t) 0 != pdw[ index ] ) )
                    index++;
            }
#ifdef OI8
            else /* 3 == width */
            {
                pqw = (uint64_t *) ram_address( get_reg_from_op( op1 ) );
                while ( ( index < limit ) && ( 0 != pqw[ index ] ) )
                    index++;
            }
#endif /* OI8 */
#endif /* OI2 */

            set_reg_from_op( op, index );
            break;
        }
        case 5: /* stoi r0address[ r1index ], 2-byte sign-extended constant */
        {
            val16 = get_word( g_oi.rpc + 2 );
            index = (oi_t) get_reg_from_op( op1 );
            val = get_reg_from_op( op );
            width = width_from_op( op1 );
            if ( 0 == width )
                set_byte( val + index, (uint8_t) val16 );
            else if_1_is_width
                set_word( val + 2 * index, val16 );
#ifndef OI2
            else if_2_is_width
                set_dword( val + 4 * index, (int32_t) (int16_t) val16 );
#ifdef OI8
            else /* 3 == width */
                set_qword( val + 8 * index, (int64_t) (int16_t) val16 );
#endif /* OI8 */
#endif /* OI2 */
        }
        case 6: /* stor r0address[ r1index ], r2value */
        {
            index = (oi_t) get_reg_from_op( op1 );
            val = get_reg_from_op( op );
            width = width_from_op( op1 );
            if ( 0 == width )
                set_byte( val + index, (uint8_t) get_reg_from_op( get_op2() ) );
            else if_1_is_width
                set_word( val + 2 * index, (uint16_t) get_reg_from_op( get_op2() ) );
#ifndef OI2
            else if_2_is_width
                set_dword( val + 4 * index, (uint32_t) get_reg_from_op( get_op2() ) );
#ifdef OI8
            else /* 3 == width */
                set_qword( val + 8 * index, get_reg_from_op( get_op2() ) );
#endif /* OI8 */
#endif /* OI2 */
        }
        case 7: /* ldor r0destination, r1address[ r2index ] */
        {
            index = (oi_t) get_reg_from_op( get_op2() );
            val = get_reg_from_op( op1 );
            width = width_from_op( op1 );
            if ( 0 == width )
                set_reg_from_op( op, (int8_t) get_byte( val + index ) );
            else if_1_is_width
                set_reg_from_op( op, (int16_t) get_word( val + 2 * index ) );
#ifndef OI2
            else if_2_is_width
                set_reg_from_op( op, (int32_t) get_dword( val + 4 * index ) );
#ifdef OI8
            else /* 3 == width */
                set_reg_from_op( op, get_qword( val + 8 * index ) );
#endif /* OI8 */
#endif /* OI2 */
        }
    }
} /* op_c0_d0_do */

uint32_t ExecuteOI()
{
#ifndef OI2
    opcode_t byte_len;
#endif /* OI2 */
    opcode_t op, op1, width;
    oi_t val;
    ioi_t ival;
    uint32_t instruction_count;

    uint8_t funct1;
    oi_t reg1;

    instruction_count = 0;

    do
    {
#ifndef NDEBUG
        assert( (oi_t) 0 == g_oi.rzero );
        assert( (oi_t) 0 == read_imgword( 0 ) );

        if ( g_OIState )
        {
            if ( g_OIState & OI_FLAG_TRACE_INSTRUCTIONS )
                TraceState();
        }
        instruction_count++;
#endif /* NDEBUG */
        op = get_op();
        switch( op )
        {
            case 0x00: { OIHalt(); goto _all_done; } /* halt */
            case 0x04: case 0x0c: case 0x10: case 0x14: case 0x18: case 0x1c: /* inc r */
            {
                inc_reg_from_op( op );
                break;
            }
            case 0x08: /* ret0: move 0 to rres and return */
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
                g_oi.rres <<= g_oi.image_shift;
                break;
            }
            case 0x40: case 0x44: case 0x4c: case 0x50: case 0x54: case 0x58: case 0x5c: /* push r */
            {
                push( get_reg_from_op( op ) );
                break;
            }
            case 0x48: /* ret0nf */
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
                g_oi.rres = IMAGE_WIDTH;
                break;
            }
            case 0x8c: case 0x90: case 0x94: case 0x98: case 0x9c: /* zero r */
            {
                set_reg_from_op( op, 0 );
                break;
            }
            case 0x88: /* shrimg */
            {
                g_oi.rres >>= g_oi.image_shift;
                break;
            }
            case 0xa0: /* addst */
            {
                pop( val );
                g_oi.rres += val;
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
            case 0xc8: /* natwid */
            {
                g_oi.rres = sizeof( oi_t );
                break;
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
                g_oi.rres &= val;
                break;
            }
            case 0x06: case 0x0a: case 0x0e: /* ld r, [address] */
            case 0x12: case 0x16: case 0x1a: case 0x1e:
            {
                set_reg_from_op( op, read_imgword( read_imgword( g_oi.rpc + 1 ) ) );
                break;
            }
            case 0x26: case 0x2a: case 0x2e: /* ldi r, value */
            case 0x32: case 0x36: case 0x3a: case 0x3e:
            {
                set_reg_from_op( op, read_imgword( g_oi.rpc + 1 ) );
                break;
            }
            case 0x42: case 0x46: case 0x4a: case 0x4e: /* st [address], r */
            case 0x52: case 0x56: case 0x5a: case 0x5e:
            {
                write_imgword( read_imgword( g_oi.rpc + 1 ), get_reg_from_op( op ) );
                break;
            }
            case 0x62: case 0x66: case 0x6a: case 0x6e: /* jmp address */
            case 0x72: case 0x76: case 0x7a: case 0x7e:
            {
                g_oi.rpc = read_imgword( g_oi.rpc + 1 ) + ( sizeof( oi_t ) * get_reg_from_op( op ) );
                continue;
            }
            case 0x82: case 0x86: case 0x8a: case 0x8e: /* inc [address] */
            case 0x92: case 0x96: case 0x9a: case 0x9e:
            {
#ifdef OI2
                ( * (oi_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )++;
#else
                if ( 2 == g_oi.image_width )
                    ( * (uint16_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )++;
                else if ( 4 == g_oi.image_width )
                    ( * (uint32_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )++;
#ifdef OI8
                else if ( 8 == g_oi.image_width )
                    ( * (uint64_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )++;
#endif /* OI8 */
#endif /* OI2 */
                break;
            }
            case 0xa2: case 0xa6: case 0xaa: case 0xae: /* dec [address] */
            case 0xb2: case 0xb6: case 0xba: case 0xbe: 
            {
#ifdef OI2
                ( * (oi_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )--;
#else
                if ( 2 == g_oi.image_width )
                    ( * (uint16_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )--;
                else if ( 4 == g_oi.image_width )
                    ( * (uint32_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )--;
#ifdef OI8
                else if ( 8 == g_oi.image_width )
                    ( * (uint64_t *) ( ram_address( read_imgword( g_oi.rpc + 1 ) + get_reg_from_op( op ) ) ) )--;
#endif /* OI8 */
#endif /* OI2 */
                break;
            }
            case 0xc2: case 0xc6: case 0xca: case 0xce: /* ldae rres (implied), address[ r ] */
            case 0xd2: case 0xd6: case 0xda: case 0xde: 
            {
                val = get_reg_from_op( op ); /* separate statement needed for HiSoft C on CP/M */
                g_oi.rres = read_imgword( read_imgword( g_oi.rpc + 1 ) + ( IMAGE_WIDTH * val ) );
                break;
            }
            case 0xe2: case 0xe6: case 0xea: case 0xee: /* call address / call reg0 + address */
            case 0xf2: case 0xf6: case 0xfa: case 0xfe: 
            {
                push( g_oi.rframe );
                push( g_oi.rpc + 1 + IMAGE_WIDTH );
                g_oi.rframe = g_oi.rsp - sizeof( oi_t ); /* point at first local variable (if any) */
                g_oi.rpc = read_imgword( g_oi.rpc + 1 ) + ( IMAGE_WIDTH * get_reg_from_op( op ) );
                continue;
            }
            case 0x03: case 0x07: case 0x0b: case 0x0f: /* j / ji / jrelb / jrel */
            case 0x13: case 0x17: case 0x1b: case 0x1f:
            {
                op1 = get_op1();
                switch( width_from_op( op1 ) )
                {
                    case 0: /* j rleft, rright, relation, offset. */
                    {
                        if ( CheckRelation( get_reg_from_op( op ), get_reg_from_op( op1 ), funct_from_op( op1 ) ) )
                        {
                            ival = (int16_t) get_word( g_oi.rpc + 2 );
                            if ( (oi_t) ival <= (oi_t) 3 )
                                jump_return( ival );
                            else
                                g_oi.rpc += ival;
                            continue;
                        }
                        break;
                    }
                    case 1: /* ji rleft, rrightCONSTANT, relation, offset. always native bit width. address extended to native width */
                    {
                        if ( CheckRelation( get_reg_from_op( op ), 1 + reg_from_op( op1 ), funct_from_op( op1 ) ) )
                        {
                            ival = (int16_t) get_word( g_oi.rpc + 2 );
                            if ( (oi_t) ival <= (oi_t) 3 )
                                jump_return( ival );
                            else
                                g_oi.rpc += ival;
                            continue;
                        }
                        break;
                    }
                    case 2: /* jrelb r0left, r1rightADDRESS, offset (from r1right), RELATION, (-128..127 pc offset) */
                    {
                        if ( CheckRelation( get_reg_from_op( op ), get_byte( get_reg_from_op( op1 ) + get_byte( g_oi.rpc + 2 ) ), funct_from_op( op1 ) ) )
                        {
                            ival = (ioi_t) (int8_t) get_byte( g_oi.rpc + 3 );
                            if ( (oi_t) ival <= (oi_t) 3 )
                                jump_return( ival );
                            else
                                g_oi.rpc += ival;
                            continue;
                        }
                        break;
                    }
                    default: /* case 3: */ /* jrel r0left, r1rightADDRESS, offset (from r1right), RELATION, (-128..127 pc offset) */
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
            case 0x47: case 0x4b: case 0x4f: /* ldinc reg0dst reg1offinc pc-relative-offset */
            case 0x53: case 0x57: case 0x5b: case 0x5f:
            {
                ldinc_do( op );
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
                        val = get_reg_from_op( op );
                        g_oi.rpc = read_imgword( g_oi.rpc + ival + ( IMAGE_WIDTH * val ) );
                        continue;
                    }
                    case 1: /* callnf address[ r0 ] */
                    {
                        push( g_oi.rpc + 4 );
                        val = get_reg_from_op( op );
                        g_oi.rpc = read_imgword( g_oi.rpc + ival + ( IMAGE_WIDTH * val ) );
                        continue;
                    }
                    default: /* case 2: */ /* callnf address */
                    {
                        push( g_oi.rpc + 4 );
                        g_oi.rpc = g_oi.rpc + ival + ( IMAGE_WIDTH * get_reg_from_op( op ) );
                        continue;
                    }
                }
            }
            case 0x83: case 0x87: case 0x8b: case 0x8f: /* sto address[ r1 ], r0 -- address is signed and pc-relative */
            case 0x93: case 0x97: case 0x9b: case 0x9f:
            {
                op1 = get_op1();
                width = width_from_op( op1 );
                val = g_oi.rpc + (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
                if ( 0 == width )
                    set_byte( val + get_reg_from_op( op1 ), (uint8_t) get_reg_from_op( op ) );
                else if_1_is_width
                    set_word( val + ( get_reg_from_op( op1 ) << 1 ), (uint16_t) get_reg_from_op( op ) );
#ifndef OI2
                else if_2_is_width
                    set_dword( val + ( get_reg_from_op( op1 ) << 2 ), (uint32_t) get_reg_from_op( op ) );
#ifdef OI8
                else /* 3 == width */
                    set_qword( val + ( get_reg_from_op( op1 ) << 3 ), get_reg_from_op( op ) );
#endif /* OI8 */
#endif /* OI2 */
                break;
            }
            case 0xa7: case 0xab: case 0xaf: /* ldo / ldob / ldoinc / ldoincb / ldiw */
            case 0xb3: case 0xb7: case 0xbb: case 0xbf:
            {
                ival = (ioi_t) (int16_t) get_word( g_oi.rpc + 2 );
                op1 = get_op1();
                funct1 = funct_from_op( op1 );
            
                if ( 2 == funct1 )
                    set_reg_from_op( op, (oi_t) ival ); /* ldiw */
                else /* funct1 is 0 or 1 for ldo variants */
                {
                    if ( 1 == funct1 ) /* pre-increment register for ldoinc variants */
                        inc_reg_from_op( op1 );
                
                    width = (uint8_t) width_from_op( op1 );
                
                    if ( (oi_t) 0 == width )
                        set_reg_from_op( op, get_byte( g_oi.rpc + ival + get_reg_from_op( op1 ) ) ); /* ldob */
                    else if_1_is_width
                    {
                        reg1 = get_reg_from_op( op1 ); /* separate statement required for HiSoft C on CP/M */
                        set_reg_from_op( op, get_word( g_oi.rpc + ival + ( reg1 << 1 ) ) ); /* ldow */
                    }
#ifndef OI2
                    else if_2_is_width
                        set_reg_from_op( op, get_dword( g_oi.rpc + ival + ( get_reg_from_op( op1 ) << 2 ) ) ); /* ldodw */
#ifdef OI8
                    else
                        set_reg_from_op( op, get_qword( g_oi.rpc + ival + ( get_reg_from_op( op1 ) << 3 ) ) ); /* ldoqw */
#endif /* OI8 */
#endif /* OI2 */
                }
                break;
            }
            case 0xc3: case 0xc7: case 0xcb: case 0xcf: /* ld / sti / stib / math / fzero / stoi / stor / ldor */
            case 0xd3: case 0xd7: case 0xdb: case 0xdf:
            {
                op_c0_d0_do( op );
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
            case 0x25: case 0x29: case 0x2d: /* cmov r0dst, r1src, funct1REL */
            case 0x31: case 0x35: case 0x39: case 0x3d:
            {
                cmov_do( op );
                break;
            }
            case 0x41: case 0x45: case 0x49: case 0x4d: /* cmpst rdst, rright, relation */
            case 0x51: case 0x55: case 0x59: case 0x5d:
            {
                /* set rdst to boolean of ( pop() RELATION rright ) */
                op1 = get_op1();
                pop( val );
                set_reg_from_op( op, (oi_t) CheckRelation( val, get_reg_from_op( op1 ), funct_from_op( op1 ) ) );
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
                        g_oi.rsp += ( sizeof( oi_t ) * ( 1 + reg_from_op( op1 ) ) );
                        continue;
                    }
                    case 3: /* ldib rdst x */
                    {
                        set_reg_from_op( op, sign_extend_oi( ( op1 & 0x1f ), 4 ) );
                        break;
                    }
                    case 4: /* signex */
                    {
                        signex_do( op );
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
#endif /* OI8 */
#endif /* OI2 */
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
#endif /* OI8 */
#endif /* OI2 */
                        break;
                    }
                    case 7: /* moddiv: push( r0 / r1 ). r0 = r0 % r1. */
                    {
                        moddiv_do( op, op1 );
                        break;
                    }
                }
                break;
            }
            case 0x81: case 0x85: case 0x89: case 0x8d: /* syscall, pushf, stst, addimgw, subimgw, addnatw, subnatw */
            case 0x91: case 0x95: case 0x99: case 0x9d:
            {
                if ( op_80_90_do( op ) )
                    continue;
                break;
            }
            case 0xa1: case 0xa5: case 0xa9: case 0xad: /* st [r0dst] r1src / ld r0dst [r1src] / pushtwo r0, r1 / poptwo r0, r1 */
            case 0xb1: case 0xb5: case 0xb9: case 0xbd:
            {
                op_a0_b0_do( op );
                break;
            }
            case 0xa3: /* cpuinfo */
            {
                g_oi.rres = 1; /* version 1 */
                g_oi.rtmp = 'd' + ( 'l' << 8 ); /* ID */
                break;
            }
            case 0xc5: case 0xc9: case 0xcd: /* mov r0dst r1src */
            case 0xd1: case 0xd5: case 0xd9: case 0xdd:
            {
                set_reg_from_op( op, get_reg_from_op( get_op1() ) );
                break;
            }
            case 0xe1: case 0xe5: case 0xe9: case 0xed: /* mathst r0dst, r1src, Math */
            case 0xf1: case 0xf5: case 0xf9: case 0xfd:
            {
                op1 = get_op1();
                pop( val );
                set_reg_from_op( op, Math( val, get_reg_from_op( op1 ), funct_from_op( op1 ) ) );
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
            byte_len = g_oi.three_byte_len;
        g_oi.rpc += (oi_t) byte_len;
#endif /* OI2 */

        continue; /* old compilers otherwise evaluate (true) each loop */
    } while ( true );

_all_done:
    return instruction_count;
} /* ExecuteOI */

