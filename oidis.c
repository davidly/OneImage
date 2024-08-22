/*
    Disassembler for OneImage
    Written by David Lee in August 2024.
    Note: there are extra, unnecessary casts and other oddities to appease older compilers.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef MSC6
#include <stdint.h>
#endif

#include "oi.h"
#include "trace.h"

#if defined( FORCETRACING ) || !defined( NDEBUG )

static const char * reg_strings[] = { "rzero", "rpc", "regsp", "rframe", "rarg1", "rarg2", "rres", "rtmp" };

#ifdef OLDCPU
static const char * RegisterString( r ) uint8_t r;
#else
static const char * RegisterString( uint8_t r )
#endif
{
    if ( r < 8 )
        return reg_strings[ r ];
    return "unknown!";
} /* RegisterString */

#ifdef OLDCPU
static const char * RegOpString( op ) uint8_t op;
#else
static const char * RegOpString( uint8_t op )
#endif
{
    return RegisterString( reg_from_op( op ) );
} /* RegOpString */

static const char * relation_strings[] = { "gt", "lt", "eq", "ne", "ge", "le" };

#ifdef OLDCPU
static const char * RelationString( r ) uint8_t r;
#else
static const char * RelationString( uint8_t r )
#endif
{
    if ( r < 6 )
        return relation_strings[ r ];
    return "unknown!";
} /* RelationString */

static const char * math_strings[] = { "add", "sub", "imul", "idiv", "or", "xor", "and", "cmp" };

#ifdef OLDCPU
static const char * MathString( r ) uint8_t r;
#else
static const char * MathString( uint8_t r )
#endif
{
    if ( r < 8 )
        return math_strings[ r ];
    return "unknown!";
} /* MathString */

static const char * syscall_strings[] = { "exit", "print string", "print integer" };

#ifdef OLDCPU
static const char * SyscallString( r ) uint8_t r;
#else
static const char * SyscallString( uint8_t r )
#endif
{
    if ( r < 3 )
        return syscall_strings[ r ];
    return "unknown!";
} /* SyscallString */

#ifdef OLDCPU
static uint8_t getbyte( pop ) uint8_t * pop;
#else
static uint8_t getbyte( uint8_t * pop )
#endif
{
    return * pop;
} /* getbyte */

#ifdef OLDCPU
static uint16_t getword( pop ) uint8_t * pop;
#else
static uint16_t getword( uint8_t * pop )
#endif
{
    return * (uint16_t *) pop;
} /* getword */

#ifdef OLDCPU
static uint32_t getdword( pop ) uint8_t * pop;
#else
static uint32_t getdword( uint8_t * pop )
#endif
{
    return * (uint32_t *) pop;
} /* getword */

#ifndef MSC6
#ifndef OLDCPU

static uint64_t getqword( uint8_t * pop )
{
    return * (uint64_t *) pop;
} /* getqword */

#endif
#endif

#ifdef OLDCPU
static const char * image_value( pop, image_width ) uint8_t * pop; uint8_t image_width;
#else
static const char * image_value( uint8_t * pop, uint8_t image_width )
#endif
{
    static char ac[ 20 ];
    ac[ 0 ] = 0;

    if ( 2 == image_width )
        sprintf( ac, "%x", getword( pop ) );
    else if ( 4 == image_width )
        sprintf( ac, "%x", getdword( pop ) );
#ifndef MSC6
#ifndef OLDCPU
    else
        sprintf( ac, "%llx", getqword( pop ) );
#endif
#endif

    return ac;
} /* image_value */

#ifdef OLDCPU
static const char * WidthSuffix( width ) uint8_t width;
#else
static const char * WidthSuffix( uint8_t width )
#endif
{
    if ( 0 == width )
        return "b";
    if ( 1 == width )
        return "w";
    if ( 2 == width )
        return "dw";
    return "qw";
} /* WidthSuffix */

#ifdef OLDCPU
static const char * relative_value( pop, rpc, val, width ) uint8_t * pop; oi_t rpc, uint8_t width;
#else
static const char * relative_value( uint8_t * pop, oi_t rpc, uint8_t width )
#endif
{
    oi_t final;
    static char ac[ 20 ];

    ac[ 0 ] = 0;
    final = rpc + (int16_t) getword( pop + 2 );

#ifndef OLDCPU
#ifndef MSC6
    if ( 3 == width )
        sprintf( ac, "%llx", (uint64_t) final );
    else
#endif
#endif
    if ( 2 == sizeof( unsigned int ) )
        sprintf( ac, "%lx", (unsigned long) final );
    else
        sprintf( ac, "%x", (unsigned int) final );
    return ac;
} /* relative_value */

#ifdef OLDCPU
const char * DisassembleOI( pop, rpc, image_width ) uint8_t * pop; oi_t rpc, uint8_t image_width;
#else
const char * DisassembleOI( uint8_t * pop, oi_t rpc, uint8_t image_width )
#endif
{
    static char buf[ 80 ];
    uint8_t op, op1, op2, op3, opOperation, byteLen, low2;
    uint8_t width, id, opfunct, op1funct;
    ioi_t ival, offset;
    const char * regstr;

    strcpy( buf, "unknown" );
    op = pop[ 0 ];
    op1 = pop[ 1 ];
    op2 = pop[ 2 ];
    op3 = pop[ 3 ];

    switch ( op )
    {
        case 0x00: { strcpy( buf, "halt" ); break; }
        case 0x04: case 0x0c: case 0x10: case 0x14: case 0x18: case 0x1c:
            { sprintf( buf, "inc %s", RegOpString( op ) ); break; }
        case 0x08: { strcpy( buf, "retzero" ); break; }
        case 0x20: { strcpy( buf, "imulst" ); break; }
        case 0x24: case 0x2c: case 0x30: case 0x34: case 0x38: case 0x3c:
            { sprintf( buf, "dec %s", RegOpString( op ) ); break; }
        case 0x28: { strcpy( buf, "shlimg" ); break; }
        case 0x40: case 0x44: case 0x4c: case 0x50: case 0x54: case 0x58: case 0x5c:
            { sprintf( buf, "push %s", RegOpString( op ) ); break; }
        case 0x48: { strcpy( buf, "retzeronf" ); break; }
        case 0x60: case 0x64: case 0x6c: case 0x70: case 0x74: case 0x78: case 0x7c:
            { sprintf( buf, "pop %s", RegOpString( op ) ); break; }
        case 0x68: { strcpy( buf, "retnf" ); break; }
        case 0x8c: case 0x90: case 0x94: case 0x98: case 0x9c:
            { sprintf( buf, "zero %s", RegOpString( op ) ); break; }
        case 0x80: { strcpy( buf, "subst" ); break; }
        case 0x84: { strcpy( buf, "imgwid" ); break; }
        case 0x88: { strcpy( buf, "shrimg" ); break; }
        case 0xa0: { strcpy( buf, "addst" ); break; }
        case 0xac: case 0xb0: case 0xb4: case 0xb8: case 0xbc:
            { sprintf( buf, "shl %s", RegOpString( op ) ); break; }
        case 0xa8: { strcpy( buf, "idivst" ); break; }
        case 0xc0: { strcpy( buf, "ret" ); break; }
        case 0xcc: case 0xd0: case 0xd4: case 0xd8: case 0xdc:
            { sprintf( buf, "shr %s", RegOpString( op ) ); break; }
        case 0xe0: { strcpy( buf, "andst" ); break; }
        case 0xec: case 0xf0: case 0xf4: case 0xf8: case 0xfc:
            { sprintf( buf, "inv %s", RegOpString( op ) ); break; }
        default:
        {
            byteLen = (uint8_t) ( op & 3 );
            if ( 0 == byteLen )
                return buf; /* unknown instruction */

            opOperation = (uint8_t) ( op & 0xe0 );

            if ( 1 == byteLen ) /* really 2 bytes */
            {
                if ( 0 == opOperation )
                    sprintf( buf,  "%s %s, %s", MathString( funct_from_op( op1 ) ), RegOpString( op ), RegOpString( op1 ) );
                else if ( 0x20 == opOperation )
                    sprintf( buf, "mov %s, %s", RegOpString( op ), RegOpString( op1 ) );
                else if ( 0x40 == opOperation ) 
                    sprintf( buf, "cmpst %s, %s, %s", RegOpString( op ), RegOpString( op1 ), RelationString( funct_from_op( op1 ) ) );
                else if ( 0x60 == opOperation )
                {
                    width = (uint8_t) width_from_op( op1 );
                    offset = (int16_t) reg_from_op( op1 );
                    op1funct = funct_from_op( op1 );

                    if ( 0 == op1funct ) /* ldf rdst, offset ( -4..3 ) */
                        sprintf( buf, "ldf%s %s, %d", WidthSuffix( width ), RegOpString( op ), (int16_t) offset );
                    else if ( 1 == op1funct ) /* stf rdst, offset ( -4..3 ) */
                        sprintf( buf, "stf%s %s, %d", WidthSuffix( width ), RegOpString( op ), (int16_t) offset );
                    else if ( 2 == op1funct ) /* ret x */
                        sprintf( buf, "ret %u", 1 + reg_from_op( op1 ) );
                    else if ( 3 == op1funct ) /* ldib r0 x */
                        sprintf( buf, "ldib %s, %d", RegOpString( op ), (int) sign_extend_oi( 0x1f & op1, 4 ) );
                    else if ( 4 == op1funct ) /* signex r0 */
                        sprintf( buf, "signex%s %s", WidthSuffix( width ), RegOpString( op ) );
                    else if ( 5 == op1funct ) /* memf */
                        sprintf( buf, "memf%s", WidthSuffix( width ) );
                    else if ( 6 == op1funct ) /* stadd */
                        sprintf( buf, "stadd%s", WidthSuffix( width ) );
                    else if ( 7 == op1funct ) /* moddiv */
                        sprintf( buf, "moddiv %s, %s", RegOpString( op ), RegOpString( op1 ) );

                }
                else if ( 0x80 == opOperation )
                {
                    offset = (int16_t) reg_from_op( op1 );
                    op1funct = (uint8_t) funct_from_op( op1 );

                    if ( 0 == op1funct ) /* syscall */
                    {
                        id = (uint8_t) ( ( ( op << 1 ) & 0x38 ) | ( ( op1 >> 2) & 7 ) );
                        sprintf( buf, "syscall %s", SyscallString( id ) );
                    }
                    else if ( 1 == op1funct ) /* pushf X */
                        sprintf( buf, "pushf %d", (int16_t) offset );
                    else if ( 2 == op1funct ) /* stst reg0 with implied [pop()] target address */
                        sprintf( buf, "stst %s", RegOpString( op ) );
                }
                else if ( 0xa0 == opOperation ) /* st [r0dst] r1src */
                {
                    width = (uint8_t) width_from_op( op1 );
                    sprintf( buf, "st%s [%s], %s", WidthSuffix( width ), RegOpString( op ), RegOpString( op1 ) );
                }
                else if ( 0xc0 == opOperation ) /* ld r0dst [r1src] */
                {
                    width = (uint8_t) width_from_op( op1 );
                    sprintf( buf, "ld%s %s, [%s]", WidthSuffix( width ), RegOpString( op ), RegOpString( op1 ) );
                }
                else if ( 0xe0 == opOperation ) /* mathst r0dst, r1src, Math */
                    sprintf( buf, "mathst %s, %s, %s", RegOpString( op ), RegOpString( op1 ), MathString( funct_from_op( op1 ) ) );
            }
            else if ( 2 == byteLen ) /* really 3 bytes */
            {
                const char * reg = RegOpString( op );
                switch ( funct_from_op( op ) )
                {
                    case 0: { sprintf( buf, "ld %s, [%s]", reg, image_value( pop + 1, image_width ) ); break; }
                    case 1: { sprintf( buf, "ldi %s, %s", reg, image_value( pop + 1, image_width ) ); break; }
                    case 2: { sprintf( buf, "st [%s], %s", image_value( pop + 1, image_width ), reg ); break; }
                    case 3:
                    {
                        if ( 0 == reg_from_op( op ) )
                            sprintf( buf, "jmp %s", image_value( pop + 1, image_width ) );
                        else
                            sprintf( buf, "jmp %s + %s", image_value( pop + 1, image_width ), reg );
                        break;
                    }
                    case 4:
                    {
                        if ( 0 == reg_from_op( op ) )
                            sprintf( buf, "inc [ %s ]", image_value( pop + 1, image_width ) );
                        else
                            sprintf( buf, "inc [ %s + %s ]", image_value( pop + 1, image_width ), reg );
                        break;
                    }
                    case 5:
                    {
                        if ( 0 == reg_from_op( op ) )
                            sprintf( buf, "dec [ %s ]", image_value( pop + 1, image_width ) );
                        else
                            sprintf( buf, "dec [ %s + %s ]", image_value( pop + 1, image_width ), reg );
                        break;
                    }
                    case 6:
                    {
                        sprintf( buf, "ldae rres, %s[ %s ]", image_value( pop + 1, image_width ), reg );
                        break;
                    }
                    case 7: { sprintf( buf, "call %s", image_value( pop + 1, image_width ) ); break; }
                }
            }
            else /* 4 bytes */
            {
                opfunct = funct_from_op( op );
                op1funct = funct_from_op( op1 );

                if ( 0 == opfunct )
                {
                    low2 = (uint8_t) width_from_op( op1 );
                    if ( 0 == low2 ) /* j rleft, rright, relation, address */
                        sprintf( buf, "j %s, %s, %s, %s", RegOpString( op ), RegOpString( op1 ),
                                 RelationString( op1funct ), relative_value( pop, rpc, image_width ) );
                    else if ( 1 == low2 ) /* ji rleft, rrightCONSTANT, relation, address */
                        sprintf( buf, "ji %s, %u, %s, %s", RegOpString( op ), 1 + reg_from_op( op1 ),
                                 RelationString( op1funct ), relative_value( pop, rpc, image_width ) );
                    else if ( 2 == low2 ) /* jrel r0left, r1rightADDRESS, offset (from r1right), RELATION (-128..127 pc offset) */
                    {
                        if ( 0 == op3 )
                            sprintf( buf, "jrelb %s, %s, %u, %s, return", RegOpString( op ), RegOpString( op1 ),
                                     op2, RelationString( op1funct ) );
                        else if ( 1 == op3 )
                            sprintf( buf, "jrelb %s, %s, %u, %s, returnnf", RegOpString( op ), RegOpString( op1 ),
                                     op2, RelationString( op1funct ) );
                        else
                            sprintf( buf, "jrelb %s, %s, %u, %s, %d", RegOpString( op ), RegOpString( op1 ),
                                     op2, RelationString( op1funct ), op3 );
                    }
                    else if ( 3 == low2 ) /* jrel r0left, r1rightADDRESS, offset (from r1right), RELATION (-128..127 pc offset) */
                        sprintf( buf, "jrel %s, %s, %u, %s, %d", RegOpString( op ), RegOpString( op1 ),
                                 op2, RelationString( op1funct ), op3 );
                }
                else if ( 1 == opfunct ) /* stinc */
                {
                    regstr = RegOpString( op );
                    width = (uint8_t) width_from_op( op1 );
                    sprintf( buf, "stinc%s [%s], %04x", WidthSuffix( width ), regstr, getword( pop + 2 ) );
                }
                else if ( 2 == opfunct ) /* ldinc */
                {
                    regstr = RegOpString( op );
                    width = (uint8_t) width_from_op( op1 );
                    sprintf( buf, "ldinc%s [%s], %s, %s", WidthSuffix( width ), regstr, RegOpString( op1 ), relative_value( pop, rpc, image_width ) );
                }
                else if ( 3 == opfunct ) /* call through function pointer table, callnf variants */
                {
                    op1funct = funct_from_op( op1 );
                    if ( op1funct < 2 )
                        sprintf( buf, "call%s %s[ %s ]", ( 0 == op1funct ) ? "" : "nf", relative_value( pop, rpc, image_width ), RegOpString( op ) );
                    else if ( 2 == op1funct )
                    {
                        if ( 0 == reg_from_op( op ) )
                            sprintf( buf, "callnf %s", relative_value( pop, rpc, image_width ) );
                        else
                            sprintf( buf, "callnf %s + %s", relative_value( pop, rpc, image_width ), RegOpString( op ) );
                    }
                }
                else if ( 4 == opfunct ) /* sto */
                {
                    width = (uint8_t) width_from_op( op1 );
                    sprintf( buf, "sto%s %s[%s], %s", WidthSuffix( width ), relative_value( pop, rpc, image_width ), RegOpString( op1 ), RegOpString( op ) );
                }
                else if ( 5 == opfunct ) /* ldo */
                {
                    width = (uint8_t) width_from_op( op1 );
                    op1funct = funct_from_op( op1 );
                    if ( 0 == op1funct )
                        sprintf( buf, "ldo%s %s, %s[%s]", WidthSuffix( width ), RegOpString( op ), relative_value( pop, rpc, image_width ), RegOpString( op1 ) );
                    else if ( 1 == op1funct )
                        sprintf( buf, "ldoinc%s %s, %s[%s]", WidthSuffix( width ), RegOpString( op ), relative_value( pop, rpc, image_width ), RegOpString( op1 ) );
                }
                else if ( 6 == opfunct ) /* ldb r0dst, [address] */
                {
                    width = (uint8_t) width_from_op( op1 );
                    op1funct = funct_from_op( op1 );
                    if ( 0 == op1funct )
                        sprintf( buf, "ld%s %s, [%04x]", WidthSuffix( width ), RegOpString( op ), getword( pop + 2 ) );
                    else if ( 1 == op1funct )
                    {
                        /* r0 has high 3 bits and r1 has low 3 bits of CONSTANT */
                        ival = (ioi_t) sign_extend_oi( (oi_t) ( ( (oi_t) reg_from_op( op ) << (oi_t) 3 ) | (oi_t) reg_from_op( op1 ) ), (oi_t) 5 );
                        sprintf( buf, "sti%s [%s], %d", WidthSuffix( width ), relative_value( pop, rpc, image_width ), (int) ival );
                    }
                    else if ( 2 == op1funct )
                        sprintf( buf, "math %s, %s, %s, %s", RegOpString( op ), RegOpString( op1 ), RegOpString( op2 ), MathString( funct_from_op( op2 ) ) );
                }
                else if ( 7 == opfunct ) /* cstf r0, r1, r1REL, r2reg */
                {
                    sprintf( buf, "cstf %s, %s, %s, %d", RegOpString( op ), RegOpString( op1 ),
                             RelationString( funct_from_op( op1 ) ),  (int16_t) reg_from_op( op2 ) );
                }
            }
        }
    }
    return buf;
} /* DisassembleOI */

#endif

