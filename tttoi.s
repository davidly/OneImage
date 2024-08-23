; register usage:
;    rarg2 is a global for the depth
; in minmax_min and minmax_max:
;    rarg1 is value and board for the win check functions
;    rtmp is i in the for loop 0..8

define x_piece 1
define o_piece 2
define blank_piece 0

define max_score 9
define min_score 2
define win_score 6
define tie_score 5
define lose_score 4

define iterations 1

define syscall_exit           0
define syscall_print_string   1
define syscall_print_integer  2

.data
    align                                   ; align with no value aligns to image width
    image_t  procs[ 9 ]
    byte     board[ 9 ]
    align
    image_t  move_count
    image_t  loop_count
    image_t  loops
    string   str_move_count " moves\n"
    string   str_iterations " iterations\n"
.dataend

.code
start:
    ldi     rtmp, iterations
    st      [loop_count], rtmp

    ldf     rarg1, 0
    ji      rarg1, 2, ne, _no_argument
    ldf     rarg1, 1
    imgwid
    add     rarg1, rres
    ld      rarg1, [rarg1]

    call    atou
    j       rres, rzero, eq, _no_argument
    st      [loop_count], rres

  _no_argument:
    ldi     rtmp, procs
    ldi     rarg1, proc0
    stinc   rtmp, rarg1
    ldi     rarg1, proc1
    stinc   rtmp, rarg1
    ldi     rarg1, proc2
    stinc   rtmp, rarg1
    ldi     rarg1, proc3
    stinc   rtmp, rarg1
    ldi     rarg1, proc4
    stinc   rtmp, rarg1
    ldi     rarg1, proc5
    stinc   rtmp, rarg1
    ldi     rarg1, proc6
    stinc   rtmp, rarg1
    ldi     rarg1, proc7
    stinc   rtmp, rarg1
    ldi     rarg1, proc8
    stinc   rtmp, rarg1

  _start_again:
    st      [move_count], rzero

    zero    rarg1
    call    runmm

    ldib    rarg1, 1
    call    runmm

    ldib    rarg1, 4
    call    runmm

    inc     [loops]
    ld      rres, [loops]
    ld      rtmp, [loop_count]
    j       rres, rtmp, ne, _start_again

    ld      rarg1, [move_count]
    syscall syscall_print_integer
    ldi     rarg1, str_move_count
    syscall syscall_print_string

    ld      rarg1, [loop_count]
    syscall syscall_print_integer
    ldi     rarg1, str_iterations
    syscall syscall_print_string

    ret                                     ; return to address 0 -- halt. both these methods work.
    syscall syscall_exit                    ; this system call does not return

atou: ; string in arg1. result in rres
        zero    rarg2                       ; running total is in rarg2

  _skipspaces:
        ldb     rtmp, [rarg1]
        ldi     rres, 32
        j       rtmp, rres, ne, _atouNext
        inc     rarg1
        jmp     _skipspaces

  _atouNext:
        ldb     rtmp, [rarg1]
        ldi     rres, 48
        j       rtmp, rres, lt, _atouDone
        ldi     rres, 57
        j       rtmp, rres, gt, _atouDone

        ldi     rres, 10
        mul     rarg2, rres

        ldi     rres, 48
        math    rtmp, rtmp, rres, sub
        add     rarg2, rtmp
        inc     rarg1
        j       rzero, rzero, eq, _atouNext

  _atouDone:
        mov     rres, rarg2
        ret

runmm:
    zero    rarg2                           ; depth
    ldi     rres, board
    add     rres, rarg1
    push    rres                            ; save this to restore the board later
    ldib    rtmp, x_piece
    stb     [ rres ], rtmp                  ; make the first move

    ldib    rres, min_score                 ; push alpha
    push    rres
    ldib    rres, max_score                 ; push beta
    push    rres

    call    minmax_min

    pop     rres
    stb     [ rres ], rzero                 ; restore the board

    ret

minmax_max:
    inc     [move_count]
    ji      rarg2, 4, lt, _max_after_winner_check

    ldib    rres, o_piece
    ldi     rarg1, board
    callnf  procs[ rtmp ]

    ji      rres, o_piece, ne, _max_after_winner_check
    ldib    rres, lose_score
    ret     2                               ; toss alpha and beta from the stack

  _max_after_winner_check:
    inc     rarg2                           ; move to the next depth
    ldib    rarg1, min_score                ; initialize value to find maximum
    ldib    rtmp, -1                        ; i in for loop 0..8

  _max_loop:
    ji      rtmp, 8, eq, _max_return_value

    ldoincb rres, board[ rtmp ]
    j       rres, rzero, ne, _max_loop

    ldib    rres, x_piece
    stob    board[ rtmp ], rres

    push    rarg1                           ; save value
    push    rtmp                            ; save for loop 0..8
    pushf   1                               ; push frame argument - alpha
    pushf   0                               ; push frame argument - beta

    call    minmax_min                      ; recurse

    pop     rtmp                            ; restore for loop 0..8
    stob    board[ rtmp ], rzero
    pop     rarg1                           ; restore value

    ji      rres, win_score, eq, _max_return ; can't do better than winning
    j       rres, rarg1, le, _max_loop      ; if score not a new high then loop

    mov     rarg1, rres                     ; update value with score
    ldf     rres, 0                         ; load beta
    j       rarg1, rres, ge, _max_return_value   ; beta pruning

    ldf     rres, 1                         ; load alpha
    j       rarg1, rres, le, _max_loop
    stf     rarg1, 1                        ; update alpha with value
    j       rzero, rzero, eq, _max_loop

  _max_return_value:
    mov     rres, rarg1
    
  _max_return:
    dec     rarg2                           ; restore depth
    ret     2                               ; toss alpha and beta from the stack

minmax_min:
    inc     [move_count]
    ji      rarg2, 4, lt, _min_after_winner_check

    ldib    rres, x_piece
    ldi     rarg1, board
    callnf  procs[ rtmp ]

    ji      rres, x_piece, ne, _min_check_tie
    ldib    rres, win_score
    ret     2                               ; toss alpha and beta from the stack

  _min_check_tie:
    ji      rarg2, 8, ne, _min_after_winner_check
    ldib    rres, tie_score
    ret     2                               ; toss alpha and beta from the stack
    
  _min_after_winner_check:
    inc     rarg2                           ; move to the next depth
    ldib    rarg1, max_score
    ldib    rtmp, -1                        ; i in for loop 0..8

  _min_loop:
    ji      rtmp, 8, eq, _min_return_value

    ldoincb rres, board[ rtmp ]
    j       rres, rzero, ne, _min_loop

    ldib    rres, o_piece
    stob    board[ rtmp ], rres

    push    rarg1                           ; save value
    push    rtmp                            ; save for loop 0..8
    pushf   1                               ; push frame argument - alpha
    pushf   0                               ; push frame argument - beta

    call    minmax_max                      ; recurse

    pop     rtmp                            ; restore for loop 0..8
    stob    board[ rtmp ], rzero
    pop     rarg1                           ; restore value

    ji      rres, lose_score, eq, _min_return ; can't do better than losing
    j       rres, rarg1, ge, _min_loop      ; if score not a new low then loop

    mov     rarg1, rres                     ; update value with score
    ldf     rres, 1                         ; load alpha
    j       rarg1, rres, le, _min_return_value   ; alpha pruning

    ldf     rres, 0                         ; load beta
    j       rarg1, rres, ge, _min_loop
    stf     rarg1, 0                        ; update beta with value
    j       rzero, rzero, eq, _min_loop

  _min_return_value:
    mov     rres, rarg1
    
  _min_return:
    dec     rarg2                           ; restore depth
    ret     2                               ; toss alpha and beta from the stack

proc0:
    jrelb   rres, rarg1, 1, ne, _proc0_next_win
    jrelb   rres, rarg1, 2, eq, retnf

  _proc0_next_win:
    jrelb   rres, rarg1, 3, ne, _proc0_next_win2
    jrelb   rres, rarg1, 6, eq, retnf

 _proc0_next_win2:
    jrelb   rres, rarg1, 4, ne, _proc0_no
    jrelb   rres, rarg1, 8, eq, retnf

 _proc0_no:
    retzeronf

proc1:
    jrelb   rres, rarg1, 0, ne, _proc1_next_win
    jrelb   rres, rarg1, 2, eq, retnf

  _proc1_next_win:
    jrelb   rres, rarg1, 4, ne, _proc1_no
    jrelb   rres, rarg1, 7, eq, retnf

 _proc1_no:
    retzeronf

proc2:
    jrelb   rres, rarg1, 0, ne, _proc2_next_win
    jrelb   rres, rarg1, 1, eq, retnf

  _proc2_next_win:
    jrelb   rres, rarg1, 5, ne, _proc2_next_win2
    jrelb   rres, rarg1, 8, eq, retnf

 _proc2_next_win2:
    jrelb   rres, rarg1, 4, ne, _proc2_no
    jrelb   rres, rarg1, 6, eq, retnf

 _proc2_no:
    retzeronf

proc3:
    jrelb   rres, rarg1, 0, ne, _proc3_next_win
    jrelb   rres, rarg1, 6, eq, retnf

  _proc3_next_win:
    jrelb   rres, rarg1, 4, ne, _proc3_no
    jrelb   rres, rarg1, 5, eq, retnf

 _proc3_no:
    retzeronf

proc4:
    jrelb   rres, rarg1, 0, ne, _proc4_next_win
    jrelb   rres, rarg1, 8, eq, retnf

  _proc4_next_win:
    jrelb   rres, rarg1, 2, ne, _proc4_next_win2
    jrelb   rres, rarg1, 6, eq, retnf

  _proc4_next_win2:
    jrelb   rres, rarg1, 1, ne, _proc4_next_win3
    jrelb   rres, rarg1, 7, eq, retnf

 _proc4_next_win3:
    jrelb   rres, rarg1, 3, ne, _proc4_no
    jrelb   rres, rarg1, 5, eq, retnf

 _proc4_no:
    retzeronf

proc5:
    jrelb   rres, rarg1, 3, ne, _proc5_next_win
    jrelb   rres, rarg1, 4, eq, retnf

  _proc5_next_win:
    jrelb   rres, rarg1, 2, ne, _proc5_no
    jrelb   rres, rarg1, 8, eq, retnf

 _proc5_no:
    retzeronf

proc6:
    jrelb   rres, rarg1, 2, ne, _proc6_next_win
    jrelb   rres, rarg1, 4, eq, retnf

  _proc6_next_win:
    jrelb   rres, rarg1, 0, ne, _proc6_next_win2
    jrelb   rres, rarg1, 3, eq, retnf

 _proc6_next_win2:
    jrelb   rres, rarg1, 7, ne, _proc6_no
    jrelb   rres, rarg1, 8, eq, retnf

 _proc6_no:
    retzeronf

proc7:
    jrelb   rres, rarg1, 1, ne, _proc7_next_win
    jrelb   rres, rarg1, 4, eq, retnf

  _proc7_next_win:
    jrelb   rres, rarg1, 6, ne, _proc7_no
    jrelb   rres, rarg1, 8, eq, retnf

 _proc7_no:
    retzeronf

proc8:
    jrelb   rres, rarg1, 0, ne, _proc8_next_win
    jrelb   rres, rarg1, 4, eq, retnf

  _proc8_next_win:
    jrelb   rres, rarg1, 2, ne, _proc8_next_win2
    jrelb   rres, rarg1, 5, eq, retnf

 _proc8_next_win2:
    jrelb   rres, rarg1, 6, ne, _proc8_no
    jrelb   rres, rarg1, 7, eq, retnf

 _proc8_no:
    retzeronf

.codeend

