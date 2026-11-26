; Byte/native loads and stores, index scaling, little-endian bytes, unaligned addresses, and bulk limits.
; Standalone self-check: prints PASS, or FAIL with the numbered assertion.
; Assemble with oia -w:2, -w:4, or -w:8. Works on all compatible oios widths.
; Expected values are literal constants, not recomputed by the operation under test.
.data
 string passed "isa_memory PASS\n"
 string failed "isa_memory FAIL assertion "
 string newline "\n"
 image_t entry_sp
 align
 byte storage[64]
.dataend
.code
    st [entry_sp],rsp

; 1: native store/load at an explicitly unaligned address
    ldi rarg1,storage
    inc rarg1
    ldi rres,4660
    st [rarg1],rres
    ld rres,[rarg1]
    ldi rtmp,1
    ldi rarg2,4660
    j rres,rarg2,ne,failure

; 2: little-endian first byte of native store
    ldi rarg1,storage
    inc rarg1
    ldb rres,[rarg1]
    ldi rtmp,2
    ldi rarg2,52
    j rres,rarg2,ne,failure

; 3: little-endian second byte of native store
    ldi rarg1,storage
    inc rarg1
    inc rarg1
    ldb rres,[rarg1]
    ldi rtmp,3
    ldi rarg2,18
    j rres,rarg2,ne,failure

; 4: byte store truncates; indirect byte load zero extends
    ldi rarg1,storage
    ldi rres,-1
    stb [rarg1],rres
    ldb rres,[rarg1]
    ldi rtmp,4
    ldi rarg2,255
    j rres,rarg2,ne,failure

; 5: byte store truncates high bits
    ldi rarg1,storage
    ldi rres,257
    stb [rarg1],rres
    ldb rres,[rarg1]
    ldi rtmp,5
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 6: native register indexed store/load scales by image width
    ldi rarg1,storage
    inc rarg1
    ldi rarg2,2
    ldi rres,-1234
    stor rarg1[rarg2],rres
    ldor rres,rarg1[rarg2]
    ldi rtmp,6
    ldi rarg2,-1234
    j rres,rarg2,ne,failure

; 7: native immediate indexed store sign extends
    ldi rarg1,storage
    inc rarg1
    ldi rarg2,3
    stoi rarg1[rarg2],-32768
    ldor rres,rarg1[rarg2]
    ldi rtmp,7
    ldi rarg2,-32768
    j rres,rarg2,ne,failure

; 8: byte indexed immediate store uses byte stride
    ldi rarg1,storage
    ldi rarg2,5
    stoib rarg1[rarg2],-7
    ldorb rres,rarg1[rarg2]
    ldi rtmp,8
    ldi rarg2,-7
    j rres,rarg2,ne,failure

; 9: byte indexed register store
    ldi rarg1,storage
    ldi rarg2,6
    ldi rres,99
    storb rarg1[rarg2],rres
    ldorb rres,rarg1[rarg2]
    ldi rtmp,9
    ldi rarg2,99
    j rres,rarg2,ne,failure

; 10: stincb advances pointer by exactly one
    ldi rarg1,storage
    mov rarg2,rarg1
    ldi rres,11
    stincb rarg1,rres
    mov rres,rarg1
    sub rres,rarg2
    ldi rtmp,10
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 11: stinc advances pointer by image width
    ldi rarg1,storage
    ldi rres,11
    stinc rarg1,rres
    ldi rres,storage
    addimgw rres
    sub rres,rarg1
    ldi rtmp,11
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 12: memfb respects nonzero start (leading sentinel)
    ldi rarg1,storage
    ldi rarg2,8
    ldi rtmp,42
    zero rres
    memfb
    ldi rres,2
    ldi rarg2,4
    ldi rtmp,7
    memfb
    ldob rres,storage[rzero]
    ldi rtmp,12
    ldi rarg2,42
    j rres,rarg2,ne,failure

; 13: memfb fills requested interval
    ldi rarg2,2
    ldob rres,storage[rarg2]
    ldi rtmp,13
    ldi rarg2,7
    j rres,rarg2,ne,failure

; 14: memfb leaves exclusive end sentinel untouched
    ldi rarg2,6
    ldob rres,storage[rarg2]
    ldi rtmp,14
    ldi rarg2,42
    j rres,rarg2,ne,failure

; 15: fzerob finds first zero from selected start
    ldi rarg1,storage
    ldi rarg2,3
    stoib rarg1[rarg2],0
    ldi rres,1
    fzerob rres,rarg1,8
    ldi rtmp,15
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 16: fzerob returns exclusive limit when absent
    ldi rarg1,storage
    zero rres
    fzerob rres,rarg1,2
    ldi rtmp,16
    ldi rarg2,2
    j rres,rarg2,ne,failure

; 17: memf fills native unaligned elements
    ldi rarg1,storage
    inc rarg1
    zero rres
    ldi rarg2,4
    ldi rtmp,-99
    memf
    ldi rarg2,2
    ldor rres,rarg1[rarg2]
    ldi rtmp,17
    ldi rarg2,-99
    j rres,rarg2,ne,failure

; 18: stadd zeroes a selected native element
    ldi rarg1,storage
    inc rarg1
    ldi rarg2,1
    ldi rtmp,1
    ldi rres,1
    stadd
    ldor rres,rarg1[rarg2]
    ldi rtmp,18
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 19: stadd preserves preceding native element
    ldi rarg1,storage
    inc rarg1
    ldor rres,rarg1[rzero]
    ldi rtmp,19
    ldi rarg2,-99
    j rres,rarg2,ne,failure

; 20: stadd preserves following native element
    ldi rarg1,storage
    inc rarg1
    ldi rarg2,2
    ldor rres,rarg1[rarg2]
    ldi rtmp,20
    ldi rarg2,-99
    j rres,rarg2,ne,failure

; 21: all tested operations restore the entry stack pointer
    ld rres,[entry_sp]
    sub rres,rsp
    ldi rtmp,21
    ldi rarg2,0
    j rres,rarg2,ne,failure

    ldi rarg1,passed
    syscall 1
    halt
failure:
    ldi rarg1,failed
    syscall 1
    mov rarg1,rtmp
    syscall 2
    ldi rarg1,newline
    syscall 1
    halt

.codeend
