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

define syscall_exit           0
define syscall_print_string   1
define syscall_print_integer  2

.data
    string str_primes " primes\n"
    byte   flags[ arraysizep ]
    align
    image_t iters
    image_t count
.dataend

.code
start:
    ldi     rtmp, loops
    st      [iters], rtmp

  loop_again:
    st      [count], rzero

    ldi     rarg1, flags
    ldib    rtmp, true
    ldi     rarg2, arraysizep
    zero    rres
    memfb                                   ; fill memory bytes at rarg1 with value of low(rtmp) for rarg2 bytes

    ; i in rarg1. prime in rarg2.
    ldi     rres, arraysize                 ; leave this here for the duration
    zero    rarg1

  next_prime:
    ldob    rtmp, flags[ rarg1 ]
    j       rtmp, rzero, eq, flag_is_off

    ldi     rarg2, 3
    add     rarg2, rarg1
    add     rarg2, rarg1

    ; k in rtmp
    math    rtmp, rarg1, rarg2, add
    j       rtmp, rres, gt, inc_count       ; redundant check to that in the kloop but this makes the loop faster

    push    rarg1                           ; save i
    ldi     rarg1, flags
    staddb                                  ; [ rtmp + rarg1 ] = 0. rtmp += rarg2. repeat while rtmp <= rres.
    pop     rarg1                           ; restore i

  inc_count:
    inc     [count]

  flag_is_off:
    inc     rarg1
    j       rarg1, rres, le, next_prime

    dec     [iters]
    ld      rtmp, iters
    j       rtmp, rzero, ne, loop_again

    ld      rarg1, [count]
    syscall syscall_print_integer
    ldi     rarg1, str_primes
    syscall syscall_print_string

    ret
.codeend

