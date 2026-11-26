; 8086 assembly to find primes like BYTE's benchmark
; build with oia:    oia sieveoi
; run with oios:     oios sieveoi
; replicate this:
;        #define TRUE 1
;        #define FALSE 0
;        #define SIZE 8190
;        
;        char flags[SIZE+1];
;        
;        int main()
;                {
;                int i,k;
;                int prime,count,iter;
;        
;                for (iter = 1; iter <= 10; iter++) {    /* do program 10 times */
;                        count = 0;                      /* initialize prime counter */
;                        for (i = 0; i <= SIZE; i++)     /* set all flags true */
;                                flags[i] = TRUE;
;                        for (i = 0; i <= SIZE; i++) {
;                                if (flags[i]) {         /* found a prime */
;                                        prime = i + i + 3;      /* twice index + 3 */
;                                        for (k = i + prime; k <= SIZE; k += prime)
;                                                flags[k] = FALSE;       /* kill all multiples */
;                                        count++;                /* primes found */
;                                        }
;                                }
;                        }
;                printf("%d primes.\n",count);           /*primes found in 10th pass */
;                return 0;
;                }
;

define true 1
define false 0

define loops 10
define arraysize 8190
define arraysizep 8191
define arraysizes 8192                      ; arraysizep plus a nonzero sentinel

define syscall_exit           0
define syscall_print_string   1
define syscall_print_integer  2

.data
    string str_primes " primes\n"
    byte   flags[ arraysizes ]
    align
    image_t iters
    image_t count
.dataend

.code
start:
    ldi     rarg1, flags                    ; use rarg1 to hold the flags array
    ldi     rtmp, loops
    st      [iters], rtmp

  loop_again:
    st      [count], rzero

    ldib    rtmp, true
    ldi     rarg2, arraysizes               ; also set the sentinel flags[ arraysizep ]
    zero    rres
    memfb                                   ; fill memory bytes at rarg1 with value of low byte of rtmp for rarg2 bytes

    ; i in rframe. prime in rarg2.
    ldi     rres, arraysize                 ; leave this here for the duration
    ldib    rframe, -1                      ; ldoincb pre-increments i

    ; phase 1: strike out multiples while the first multiple k = 3i + 3 is in the array.
  next_prime:
    ldoincb rtmp, flags[ rframe ]           ; i++, rtmp = flags[ i ]
    j       rtmp, rzero, eq, next_prime

    ldib    rarg2, 3
    add     rarg2, rframe
    add     rarg2, rframe

    ; k in rtmp
    math    rtmp, rframe, rarg2, add
    j       rtmp, rres, gt, count_prime     ; k > SIZE for this and every later i, so only counting is left

    staddb                                  ; rarg1[ rtmp ] = 0. rtmp += rarg2. repeat while rtmp <= rres.
    inc     [count]
    j       rzero, rzero, eq, next_prime

    ; phase 2: count the remaining primes. the sentinel stops the scan at the end of the array.
  count_prime:
    inc     [count]

  count_next:
    ldoincb rtmp, flags[ rframe ]
    j       rtmp, rzero, eq, count_next
    j       rframe, rres, le, count_prime   ; i > SIZE means this is the sentinel

    dec     [iters]
    ld      rtmp, iters
    j       rtmp, rzero, ne, loop_again

    ld      rarg1, [count]
    syscall syscall_print_integer
    ldi     rarg1, str_primes
    syscall syscall_print_string

    ret
.codeend

