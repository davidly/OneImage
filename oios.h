/*  OneImage OS 
    Written by David Lee in August 2024.
*/

/* this is the executable file header */

struct OIHeader
{
    uint8_t sig0;               /* O */
    uint8_t sig1;               /* I */
    uint8_t version;
    uint8_t flags;              /* lower two bits: 00 16-bit. 01: 32-bit. 10: 64-bit image width */
    uint32_t unused;            /* for future use and to get the header to a multiple of 8 bytes */
    uint32_t cbCode;            /* count of bytes for code. code in file begins after the header */
    uint32_t cbInitializedData; /* count of bytes for initialized data. initialized data in file begins just after code */
    uint32_t cbZeroFilledData;  /* count of bytes for zero-filled data. brk is set immediately after this */
    uint32_t cbStack;           /* # of bytes required for stack from top of RAM down to end of zero-filled data */
    uint32_t loRamRequired;     /* 0 means as much as is reasonably available. generally the sum of the prior 4 fields */
    uint32_t hiRamRequired;     /* only used when image width is 8 */
    uint32_t loInitialPC;       /* 0-based offset from start of code; usually 2 * image width */
    uint32_t hiInitialPC;       /* only used when image width is 8 */

    /* code loaded at address 0, includes syscall adddress */
    /* initialized data loaded immediately after code, and should be at least image width aligned */
};


