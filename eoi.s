; OI assembly to compute e to n digits.
; When DIGITS is 200, the first 192 digits are displayed:
; 271828182845904523536028747135266249775724709369995957496696762772407663035354759457138217852516642742746639193200305992181741359662904357290033429526059563073813232862794349076323382988075319
;
; equivalent to:
;     program e;
;     const
;        DIGITS = 200;
;     type
;         arrayType = array[ 0..DIGITS ] of integer;
;     var 
;         high, n, x : integer;
;         a : arrayType;
;     begin
;         high := DIGITS;
;         x := 0;
;         n := high - 1;
;         while n > 0 do begin
;             a[ n ] := 1;
;             n := n - 1;
;         end;
;     
;         a[ 1 ] := 2;
;         a[ 0 ] := 0;
;     
;         while high > 9 do begin
;             high := high - 1;
;             n := high;
;             while 0 <> n do begin
;                 a[ n ] := x MOD n;
;                 x := 10 * a[ n - 1 ] + x DIV n;
;                 n := n - 1;
;             end;
;     
;             Write( x );
;         end;
;     
;         writeln; writeln( 'done' );
;     end.
;
; build with oia:    oia eoi
; run with oios:     oios eoi

define true 1
define false 0
define digits 200

define syscall_exit           0
define syscall_print_string   1
define syscall_print_integer  2

.data
    string str_done "\ndone\n"
    byte   array[ digits ]
.dataend

.code
start:
    ; while n > 0 do begin
    ;     a[ n ] := 1;
    ;     n := n - 1;
    ; end;

    ldi     rarg1, array
    ldib    rtmp, 1
    ldi     rarg2, digits
    zero    rres
    memfb

    ; a[ 1 ] := 2;
    ; a[ 0 ] := 0;

    ldi     rarg1, array
    stincb  [rarg1], 0
    stincb  [rarg1], 2

    ; rarg2 is n. rtmp is high. rres is x
    ldi     rtmp, digits
    zero    rres

  _outerloop:
    dec     rtmp
    mov     rarg2, rtmp

  _innerloop:
    moddiv  rres, rarg2                  ; mod is in rres. dividend is on the stack
    stob    array[ rarg2 ], rres         ; a[ n ] = x MOD n
    dec     rarg2                        ; n--
    ldob    rres, array[ rarg2 ]
    ldiw    rarg1, 10
    imul    rres, rarg1                  ; 10 * a[ n - 1 ]
    mathst  rres, rres, add              ; add results of multiplication and division
    j       rarg2, rzero, ne, _innerloop

    mov     rarg1, rres
    syscall syscall_print_integer

    ldiw    rarg1, 9
    j       rtmp, rarg1, gt, _outerloop

    ldi     rarg1, str_done
    syscall syscall_print_string

    ret                                  ; return to address 0
.codeend
