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
    string  str_fail_1 "failure 1"
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

    ldi     rarg1, str_done
    syscall syscall_print_string

    ret

test_fail_1:
    ldi     rarg1, str_fail_1
    call    failure

failure:
    syscall syscall_print_string
    syscall syscall_exit
.codeend

