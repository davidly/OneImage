; OneImage tests

define true 1
define false 0

define syscall_exit           0
define syscall_print_string   1
define syscall_print_integer  2

define array_size 20

.data
    string  str_done "done\n"
    string  str_argc " argc\n"
    string  str_nl "\n"
    byte    byte_array[ array_size ]
    word    word_array[ array_size ]
    image_t native_array[ array_size ]
    string  str_failure "failure in test "
.dataend

.code
start:

    ldf     rarg1, 0
    syscall syscall_print_integer
    ldi     rarg1, str_argc
    syscall syscall_print_string

    ldf     rtmp, 0
    ldf     rarg2, 1

  _next_arg:
    ld      rarg1, [rarg2]
    syscall syscall_print_string
    addimgw rarg2
    push    rarg1
    ldi     rarg1, str_nl
    syscall syscall_print_string
    pop     rarg1
    dec     rtmp
    j       rtmp, rzero, ne, _next_arg

    zero    rres
    zero    rtmp
    zero    rarg1
    zero    rarg2

    ldib    rres, -1
    ldi     rarg2, 255
    signexb rarg2
    j       rres, rarg2, ne, test_fail_1

    ldi     rarg1, 17
    ldi     rarg2, 5
    mov     rres, rarg1
    add     rres, rarg2
    ldi     rtmp, 22
    j       rres, rtmp, ne, test_fail_2

    mov     rres, rarg1
    sub     rres, rarg2
    ldi     rtmp, 12
    j       rres, rtmp, ne, test_fail_3

    mov     rres, rarg1
    mul     rres, rarg2
    ldi     rtmp, 85
    j       rres, rtmp, ne, test_fail_4

    mov     rres, rarg1
    div     rres, rarg2
    ldi     rtmp, 3
    j       rres, rtmp, ne, test_fail_5

    mov     rres, rarg1
    or      rres, rarg2
    ldi     rtmp, 21
    j       rres, rtmp, ne, test_fail_6

    mov     rres, rarg1
    xor     rres, rarg2
    ldi     rtmp, 20
    j       rres, rtmp, ne, test_fail_7

    mov     rres, rarg1
    and     rres, rarg2
    ldi     rtmp, 1
    j       rres, rtmp, ne, test_fail_8

    mov     rres, rarg1
    cmp     rres, rarg2                  ; true if !=, false if =. ( 0 != ( left - right ) )
    ldi     rtmp, 1
    j       rres, rtmp, ne, test_fail_9

    ldib    rtmp, -3
    ldi     rarg1, -3 
    j       rtmp, rarg1, ne, test_fail_10
    ldi     rarg2, -300000
    ldi     rres, 100000
    div     rarg2, rres
    imgwid
    ldib    rarg1, 2
    j       rres, rarg1, eq, _width_2_ignore
    j       rtmp, rarg2, ne, test_fail_11
  _width_2_ignore:

    zero    rtmp
    ldi     rres, 20
    cmov    rres, rtmp, le
    j       rres, rzero, eq, test_fail_12

    ldi     rres, -39
    cmov    rres, rtmp, le
    j       rres, rzero, ne, test_fail_12

    ldi     rarg1, str_done
    syscall syscall_print_string

    ret

test_fail_1:
    ldi    rarg1, 1
    jmp    failure

test_fail_2:
    ldi    rarg1, 2
    jmp    failure

test_fail_3:
    ldi    rarg1, 3
    jmp    failure

test_fail_4:
    ldi    rarg1, 4
    jmp    failure

test_fail_5:
    ldi    rarg1, 5
    jmp    failure

test_fail_6:
    ldi    rarg1, 6
    jmp    failure

test_fail_7:
    ldi    rarg1, 7
    jmp    failure

test_fail_8:
    ldi    rarg1, 8
    jmp    failure

test_fail_9:
    ldi    rarg1, 9
    jmp    failure

test_fail_10:
    ldi    rarg1, 10
    jmp    failure

test_fail_11:
    ldi    rarg1, 11
    jmp    failure

test_fail_12:
    ldi    rarg1, 12
    jmp    failure

test_fail_13:
    ldi    rarg1, 13
    jmp    failure

failure:
    mov    rarg2, rarg1
    ldi    rarg1, str_failure
    syscall syscall_print_string
    mov    rarg1, rarg2
    syscall syscall_print_integer
    ldi    rarg1, str_nl
    syscall syscall_print_string

    syscall syscall_exit
.codeend

