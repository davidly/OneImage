; Integer ALU, operand ordering, signed division, shifts, and stack balance.
; Standalone self-check: prints PASS, or FAIL with the numbered assertion.
; Assemble with oia -w:2, -w:4, or -w:8. Works on all compatible oios widths.
; Expected values are literal constants, not recomputed by the operation under test.
.data
 string passed "isa_arithmetic PASS\n"
 string failed "isa_arithmetic FAIL assertion "
 string newline "\n"
 image_t entry_sp

 image_t saved
.dataend
.code
    st [entry_sp],rsp

; 1: add 17, 5
    ldi rres,17
    ldi rarg1,5
    add rres,rarg1
    ldi rtmp,1
    ldi rarg2,22
    j rres,rarg2,ne,failure

; 2: add -17, 5
    ldi rres,-17
    ldi rarg1,5
    add rres,rarg1
    ldi rtmp,2
    ldi rarg2,-12
    j rres,rarg2,ne,failure

; 3: add 0, 0
    ldi rres,0
    ldi rarg1,0
    add rres,rarg1
    ldi rtmp,3
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 4: sub 17, 5
    ldi rres,17
    ldi rarg1,5
    sub rres,rarg1
    ldi rtmp,4
    ldi rarg2,12
    j rres,rarg2,ne,failure

; 5: sub 5, 17
    ldi rres,5
    ldi rarg1,17
    sub rres,rarg1
    ldi rtmp,5
    ldi rarg2,-12
    j rres,rarg2,ne,failure

; 6: sub -17, -5
    ldi rres,-17
    ldi rarg1,-5
    sub rres,rarg1
    ldi rtmp,6
    ldi rarg2,-12
    j rres,rarg2,ne,failure

; 7: imul -17, 5
    ldi rres,-17
    ldi rarg1,5
    imul rres,rarg1
    ldi rtmp,7
    ldi rarg2,-85
    j rres,rarg2,ne,failure

; 8: imul -17, -5
    ldi rres,-17
    ldi rarg1,-5
    imul rres,rarg1
    ldi rtmp,8
    ldi rarg2,85
    j rres,rarg2,ne,failure

; 9: imul 32767, 0
    ldi rres,32767
    ldi rarg1,0
    imul rres,rarg1
    ldi rtmp,9
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 10: idiv 17, 5
    ldi rres,17
    ldi rarg1,5
    idiv rres,rarg1
    ldi rtmp,10
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 11: idiv -17, 5
    ldi rres,-17
    ldi rarg1,5
    idiv rres,rarg1
    ldi rtmp,11
    ldi rarg2,-3
    j rres,rarg2,ne,failure

; 12: idiv 17, -5
    ldi rres,17
    ldi rarg1,-5
    idiv rres,rarg1
    ldi rtmp,12
    ldi rarg2,-3
    j rres,rarg2,ne,failure

; 13: idiv -17, -5
    ldi rres,-17
    ldi rarg1,-5
    idiv rres,rarg1
    ldi rtmp,13
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 14: idiv 1, 2
    ldi rres,1
    ldi rarg1,2
    idiv rres,rarg1
    ldi rtmp,14
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 15: idiv 17, 0
    ldi rres,17
    ldi rarg1,0
    idiv rres,rarg1
    ldi rtmp,15
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 16: and 170, 85
    ldi rres,170
    ldi rarg1,85
    and rres,rarg1
    ldi rtmp,16
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 17: and 170, 240
    ldi rres,170
    ldi rarg1,240
    and rres,rarg1
    ldi rtmp,17
    ldi rarg2,160
    j rres,rarg2,ne,failure

; 18: or 170, 85
    ldi rres,170
    ldi rarg1,85
    or rres,rarg1
    ldi rtmp,18
    ldi rarg2,255
    j rres,rarg2,ne,failure

; 19: xor 170, 85
    ldi rres,170
    ldi rarg1,85
    xor rres,rarg1
    ldi rtmp,19
    ldi rarg2,255
    j rres,rarg2,ne,failure

; 20: xor 170, 170
    ldi rres,170
    ldi rarg1,170
    xor rres,rarg1
    ldi rtmp,20
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 21: addst pops the left operand; rres is the right operand
    ldi rarg1,17
    push rarg1
    ldi rres,-5
    addst
    ldi rtmp,21
    ldi rarg2,12
    j rres,rarg2,ne,failure

; 22: subst pops the left operand; rres is the right operand
    ldi rarg1,17
    push rarg1
    ldi rres,5
    subst
    ldi rtmp,22
    ldi rarg2,12
    j rres,rarg2,ne,failure

; 23: subst pops the left operand; rres is the right operand
    ldi rarg1,5
    push rarg1
    ldi rres,17
    subst
    ldi rtmp,23
    ldi rarg2,-12
    j rres,rarg2,ne,failure

; 24: imulst pops the left operand; rres is the right operand
    ldi rarg1,-17
    push rarg1
    ldi rres,5
    imulst
    ldi rtmp,24
    ldi rarg2,-85
    j rres,rarg2,ne,failure

; 25: idivst pops the left operand; rres is the right operand
    ldi rarg1,-17
    push rarg1
    ldi rres,5
    idivst
    ldi rtmp,25
    ldi rarg2,-3
    j rres,rarg2,ne,failure

; 26: idivst pops the left operand; rres is the right operand
    ldi rarg1,17
    push rarg1
    ldi rres,-5
    idivst
    ldi rtmp,26
    ldi rarg2,-3
    j rres,rarg2,ne,failure

; 27: andst pops the left operand; rres is the right operand
    ldi rarg1,170
    push rarg1
    ldi rres,240
    andst
    ldi rtmp,27
    ldi rarg2,160
    j rres,rarg2,ne,failure

; 28: shift left
    ldi rres,129
    shl rres
    ldi rtmp,28
    ldi rarg2,258
    j rres,rarg2,ne,failure

; 29: logical shift right
    ldi rres,258
    shr rres
    ldi rtmp,29
    ldi rarg2,129
    j rres,rarg2,ne,failure

; 30: inv zero is true
    zero rres
    inv rres
    ldi rtmp,30
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 31: inv nonzero is false
    ldi rres,-1
    inv rres
    ldi rtmp,31
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 32: signed byte boundary
    ldi rres,128
    signexb rres
    ldi rtmp,32
    ldi rarg2,-128
    j rres,rarg2,ne,failure

; 33: signed byte maximum
    ldi rres,127
    signexb rres
    ldi rtmp,33
    ldi rarg2,127
    j rres,rarg2,ne,failure

; 34: signed word boundary
    ldi rres,32768
    signexw rres
    ldi rtmp,34
    ldi rarg2,-32768
    j rres,rarg2,ne,failure

; 35: signed word all ones
    ldi rres,65535
    signexw rres
    ldi rtmp,35
    ldi rarg2,-1
    j rres,rarg2,ne,failure

; 36: compact immediate minimum
    ldib rres,-16
    ldi rtmp,36
    ldi rarg2,-16
    j rres,rarg2,ne,failure

; 37: signed immediate word
    ldiw rres,-32768
    ldi rtmp,37
    ldi rarg2,-32768
    j rres,rarg2,ne,failure

; 38: swap preserves both values
    ldi rarg1,19
    ldi rres,23
    swap rarg1,rres
    sub rres,rarg1
    ldi rtmp,38
    ldi rarg2,-4
    j rres,rarg2,ne,failure

; 39: pop preserves LIFO order
    ldi rarg1,19
    push rarg1
    ldi rarg1,23
    push rarg1
    pop rres
    pop rarg1
    sub rres,rarg1
    ldi rtmp,39
    ldi rarg2,4
    j rres,rarg2,ne,failure

; 40: balanced ALU stack operations restore rsp
    mov rarg1,rsp
    push rarg1
    pop rres
    sub rres,rsp
    ldi rtmp,40
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 41: moddiv stores remainder and pushes unsigned quotient
    ldi rres,29
    ldi rarg1,6
    moddiv rres,rarg1
    pop rarg1
    st [saved],rres
    mov rres,rarg1
    ldi rtmp,41
    ldi rarg2,4
    j rres,rarg2,ne,failure

; 42: moddiv remainder from previous assertion
    ld rres,[saved]
    ldi rtmp,42
    ldi rarg2,5
    j rres,rarg2,ne,failure

; 43: all tested operations restore the entry stack pointer
    ld rres,[entry_sp]
    sub rres,rsp
    ldi rtmp,43
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
