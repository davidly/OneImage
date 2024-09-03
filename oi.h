/*  OneImage interpreter
    Written by David Lee in August 2024.

    oddities for old compilers:
       -- #if defined() doesn't work
       -- no #elif or #elifdef
       -- #define must be in first column for HISOFTCPM
       -- function prototypes are non-standard
       -- must use #define, not inlined functions for perf and syntax reasons
       -- no const, assert(), __assume()
       -- no 8-byte integers
*/

#ifdef MSC6

    typedef unsigned char uint8_t;
    typedef char int8_t;
    typedef unsigned int uint16_t;
    typedef int int16_t;
    typedef unsigned long uint32_t;
    typedef long int32_t;
    typedef unsigned int size_t;
    
    #define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )

#endif /* MSC6 */

#ifdef AZTECCPM

    typedef unsigned char uint8_t;
    typedef char int8_t;
    typedef unsigned int uint16_t;
    typedef int int16_t;
    typedef unsigned long uint32_t;
    typedef long int32_t;
    typedef int size_t;
    
    #define assert( x )
    
#endif /* AZTECCPM */

#ifdef HISOFTCPM
#define const
#endif

#define sign_extend_oi( x, bits ) ( ( (x) ^ ( (oi_t) 1 << bits ) ) - ( ( (oi_t) 1 ) << bits ) )

#ifdef OI2

    typedef uint16_t oi_t;
    typedef int16_t ioi_t;

    struct OneImage
    {
        uint16_t rzero;   /* always 0 */
        uint16_t a_rzero; /* alt registers enable use of reg from opcodes to be used without a shift */
        uint16_t rpc;     /* program counter */
        uint16_t a_rpc;
        uint16_t rsp;     /* stack pointer */
        uint16_t a_rsp;
        uint16_t rframe;  /* points to first local on the stack if any */
        uint16_t a_rframe;
        uint16_t rarg1;
        uint16_t a_rarg1;
        uint16_t rarg2;
        uint16_t a_rarg2;
        uint16_t rres;    /* by convention, where functions return results. 3rd argument if needed */
        uint16_t a_rres;
        uint16_t rtmp;    /* 4th argument if needed */
        uint16_t a_rtmp;
        uint8_t image_width; /* 2, 4, or 8 */
        uint8_t image_shift; /* 1, 2, or 3 */
        uint8_t three_byte_len; /* 1 + image_width */
    };

#else /* OI2 */

#ifdef OI4
    typedef uint32_t oi_t;
    typedef int32_t ioi_t;
#else /* OI4 */
    typedef uint64_t oi_t;
    typedef int64_t ioi_t;
#endif /* OI4 */

    struct OneImage
    {
        oi_t rzero;   /* always 0 */
        oi_t rpc;     /* program counter */
        oi_t rsp;     /* stack pointer */
        oi_t rframe;  /* points to first local on the stack if any. if callnf/retnf used can be general-purpose */
        oi_t rarg1;   /* first argument */
        oi_t rarg2;   /* second argument */
        oi_t rres;    /* by convention, where functions return results. 3rd argument if needed */
        oi_t rtmp;    /* 4th argument if needed */
        oi_t address_mask; /* generally used to mask addresses when image width < native width */
        uint8_t image_width; /* 2, 4, or 8 */
        uint8_t image_shift; /* 1, 2, or 3 */
        uint8_t three_byte_len; /* 1 + image_width */
    };

#endif /* OI2 */

#ifdef __GNUC__
    #define cdecl
    #define __forceinline
#else
    typedef size_t bool;
#endif /* __GNUC__ */

extern struct OneImage g_oi;

#ifdef HISOFTCPM
    extern uint32_t RamInformationOI( uint32_t, uint8_t **, uint8_t );
    extern void ResetOI( oi_t, oi_t, oi_t, uint8_t );
    extern uint32_t ExecuteOI( void );
    extern void TraceInstructionsOI( int );
    extern char * DisassembleOI( uint8_t *, oi_t, uint8_t );
    
    extern void OISyscall( size_t );
    extern void OIHalt( void );
    extern void OIHardTermination( void );
#else /* HISOFTCPM */
#ifdef AZTECCPM
    extern uint32_t RamInformationOI();
    extern void ResetOI();
    extern uint32_t ExecuteOI();
    extern void TraceInstructionsOI( );
    extern char * DisassembleOI();
    
    extern void OISyscall();
    extern void OIHalt();
    extern void OIHardTermination();
#else
    extern uint32_t RamInformationOI( uint32_t required, uint8_t ** ppRam, uint8_t image_width );
    extern void ResetOI( oi_t mem_size, oi_t pc, oi_t sp, uint8_t image_width );
    extern uint32_t ExecuteOI( void );
    extern void TraceInstructionsOI( bool trace );
    extern const char * DisassembleOI( uint8_t * pop, oi_t rpc, uint8_t image_width );
    
    extern void OISyscall( size_t function );
    extern void OIHalt( void );
    extern void OIHardTermination( void );
#endif /* AZTECCPM */
#endif /* HISOFTCPM */

#ifdef _MSC_VER
#if _MSC_VER <= 600
#define __assume( x )
#define __forceinline
#endif /* _MSC_VER <= 600 */
#else
#define __assume( x )
#endif /* _MSC_VER */

#ifdef WATCOM
#define __forceinline
#endif /* WATCOM */

/* opcode decoding utilities */

#define funct_from_op( op ) ( (uint8_t) ( op >> 5 ) )
#define reg_from_op( op ) ( (uint8_t) ( ( op >> 2 ) & 7 ) )
#define width_from_op( op ) ( op & 3 )
#define byte_len_from_op( op ) ( op & 3 )

