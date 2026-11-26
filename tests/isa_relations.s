; Signed relations and parity, branch taken/not taken, cmp, cmpst, and cmov.
; Standalone self-check: prints PASS, or FAIL with the numbered assertion.
; Assemble with oia -w:2, -w:4, or -w:8. Works on all compatible oios widths.
; Expected values are literal constants, not recomputed by the operation under test.
.data
 string passed "isa_relations PASS\n"
 string failed "isa_relations FAIL assertion "
 string newline "\n"
 image_t entry_sp

.dataend
.code
    st [entry_sp],rsp

; 1: cmp -3 gt 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,gt
    ldi rtmp,1
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 2: cmpst -3 gt 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,gt
    ldi rtmp,2
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 3: j -3 gt 2 (expect fallthrough)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,gt,taken3
    jmp after3
    taken3:
    ldi rres,1
    after3:
    ldi rtmp,3
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 4: cmp 2 gt -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,gt
    ldi rtmp,4
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 5: cmpst 2 gt -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,gt
    ldi rtmp,5
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 6: j 2 gt -3 (expect taken)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,gt,taken6
    jmp after6
    taken6:
    ldi rres,1
    after6:
    ldi rtmp,6
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 7: cmp 2 gt 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,gt
    ldi rtmp,7
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 8: cmpst 2 gt 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,gt
    ldi rtmp,8
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 9: j 2 gt 2 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,gt,taken9
    jmp after9
    taken9:
    ldi rres,1
    after9:
    ldi rtmp,9
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 10: cmp -2 gt -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,gt
    ldi rtmp,10
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 11: cmpst -2 gt -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,gt
    ldi rtmp,11
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 12: j -2 gt -2 (expect fallthrough)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,gt,taken12
    jmp after12
    taken12:
    ldi rres,1
    after12:
    ldi rtmp,12
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 13: cmp -3 lt 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,lt
    ldi rtmp,13
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 14: cmpst -3 lt 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,lt
    ldi rtmp,14
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 15: j -3 lt 2 (expect taken)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,lt,taken15
    jmp after15
    taken15:
    ldi rres,1
    after15:
    ldi rtmp,15
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 16: cmp 2 lt -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,lt
    ldi rtmp,16
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 17: cmpst 2 lt -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,lt
    ldi rtmp,17
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 18: j 2 lt -3 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,lt,taken18
    jmp after18
    taken18:
    ldi rres,1
    after18:
    ldi rtmp,18
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 19: cmp 2 lt 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,lt
    ldi rtmp,19
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 20: cmpst 2 lt 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,lt
    ldi rtmp,20
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 21: j 2 lt 2 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,lt,taken21
    jmp after21
    taken21:
    ldi rres,1
    after21:
    ldi rtmp,21
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 22: cmp -2 lt -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,lt
    ldi rtmp,22
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 23: cmpst -2 lt -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,lt
    ldi rtmp,23
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 24: j -2 lt -2 (expect fallthrough)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,lt,taken24
    jmp after24
    taken24:
    ldi rres,1
    after24:
    ldi rtmp,24
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 25: cmp -3 eq 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,eq
    ldi rtmp,25
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 26: cmpst -3 eq 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,eq
    ldi rtmp,26
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 27: j -3 eq 2 (expect fallthrough)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,eq,taken27
    jmp after27
    taken27:
    ldi rres,1
    after27:
    ldi rtmp,27
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 28: cmp 2 eq -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,eq
    ldi rtmp,28
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 29: cmpst 2 eq -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,eq
    ldi rtmp,29
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 30: j 2 eq -3 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,eq,taken30
    jmp after30
    taken30:
    ldi rres,1
    after30:
    ldi rtmp,30
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 31: cmp 2 eq 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,eq
    ldi rtmp,31
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 32: cmpst 2 eq 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,eq
    ldi rtmp,32
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 33: j 2 eq 2 (expect taken)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,eq,taken33
    jmp after33
    taken33:
    ldi rres,1
    after33:
    ldi rtmp,33
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 34: cmp -2 eq -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,eq
    ldi rtmp,34
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 35: cmpst -2 eq -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,eq
    ldi rtmp,35
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 36: j -2 eq -2 (expect taken)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,eq,taken36
    jmp after36
    taken36:
    ldi rres,1
    after36:
    ldi rtmp,36
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 37: cmp -3 ne 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,ne
    ldi rtmp,37
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 38: cmpst -3 ne 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,ne
    ldi rtmp,38
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 39: j -3 ne 2 (expect taken)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,ne,taken39
    jmp after39
    taken39:
    ldi rres,1
    after39:
    ldi rtmp,39
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 40: cmp 2 ne -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,ne
    ldi rtmp,40
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 41: cmpst 2 ne -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,ne
    ldi rtmp,41
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 42: j 2 ne -3 (expect taken)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,ne,taken42
    jmp after42
    taken42:
    ldi rres,1
    after42:
    ldi rtmp,42
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 43: cmp 2 ne 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,ne
    ldi rtmp,43
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 44: cmpst 2 ne 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,ne
    ldi rtmp,44
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 45: j 2 ne 2 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,ne,taken45
    jmp after45
    taken45:
    ldi rres,1
    after45:
    ldi rtmp,45
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 46: cmp -2 ne -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,ne
    ldi rtmp,46
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 47: cmpst -2 ne -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,ne
    ldi rtmp,47
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 48: j -2 ne -2 (expect fallthrough)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,ne,taken48
    jmp after48
    taken48:
    ldi rres,1
    after48:
    ldi rtmp,48
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 49: cmp -3 ge 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,ge
    ldi rtmp,49
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 50: cmpst -3 ge 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,ge
    ldi rtmp,50
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 51: j -3 ge 2 (expect fallthrough)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,ge,taken51
    jmp after51
    taken51:
    ldi rres,1
    after51:
    ldi rtmp,51
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 52: cmp 2 ge -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,ge
    ldi rtmp,52
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 53: cmpst 2 ge -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,ge
    ldi rtmp,53
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 54: j 2 ge -3 (expect taken)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,ge,taken54
    jmp after54
    taken54:
    ldi rres,1
    after54:
    ldi rtmp,54
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 55: cmp 2 ge 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,ge
    ldi rtmp,55
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 56: cmpst 2 ge 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,ge
    ldi rtmp,56
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 57: j 2 ge 2 (expect taken)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,ge,taken57
    jmp after57
    taken57:
    ldi rres,1
    after57:
    ldi rtmp,57
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 58: cmp -2 ge -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,ge
    ldi rtmp,58
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 59: cmpst -2 ge -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,ge
    ldi rtmp,59
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 60: j -2 ge -2 (expect taken)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,ge,taken60
    jmp after60
    taken60:
    ldi rres,1
    after60:
    ldi rtmp,60
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 61: cmp -3 le 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,le
    ldi rtmp,61
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 62: cmpst -3 le 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,le
    ldi rtmp,62
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 63: j -3 le 2 (expect taken)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,le,taken63
    jmp after63
    taken63:
    ldi rres,1
    after63:
    ldi rtmp,63
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 64: cmp 2 le -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,le
    ldi rtmp,64
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 65: cmpst 2 le -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,le
    ldi rtmp,65
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 66: j 2 le -3 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,le,taken66
    jmp after66
    taken66:
    ldi rres,1
    after66:
    ldi rtmp,66
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 67: cmp 2 le 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,le
    ldi rtmp,67
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 68: cmpst 2 le 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,le
    ldi rtmp,68
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 69: j 2 le 2 (expect taken)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,le,taken69
    jmp after69
    taken69:
    ldi rres,1
    after69:
    ldi rtmp,69
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 70: cmp -2 le -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,le
    ldi rtmp,70
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 71: cmpst -2 le -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,le
    ldi rtmp,71
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 72: j -2 le -2 (expect taken)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,le,taken72
    jmp after72
    taken72:
    ldi rres,1
    after72:
    ldi rtmp,72
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 73: cmp -3 even 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,even
    ldi rtmp,73
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 74: cmpst -3 even 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,even
    ldi rtmp,74
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 75: j -3 even 2 (expect fallthrough)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,even,taken75
    jmp after75
    taken75:
    ldi rres,1
    after75:
    ldi rtmp,75
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 76: cmp 2 even -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,even
    ldi rtmp,76
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 77: cmpst 2 even -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,even
    ldi rtmp,77
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 78: j 2 even -3 (expect taken)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,even,taken78
    jmp after78
    taken78:
    ldi rres,1
    after78:
    ldi rtmp,78
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 79: cmp 2 even 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,even
    ldi rtmp,79
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 80: cmpst 2 even 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,even
    ldi rtmp,80
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 81: j 2 even 2 (expect taken)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,even,taken81
    jmp after81
    taken81:
    ldi rres,1
    after81:
    ldi rtmp,81
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 82: cmp -2 even -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,even
    ldi rtmp,82
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 83: cmpst -2 even -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,even
    ldi rtmp,83
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 84: j -2 even -2 (expect taken)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,even,taken84
    jmp after84
    taken84:
    ldi rres,1
    after84:
    ldi rtmp,84
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 85: cmp -3 odd 2
    ldi rarg1,-3
    ldi rarg2,2
    cmp rres,rarg1,rarg2,odd
    ldi rtmp,85
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 86: cmpst -3 odd 2
    ldi rarg1,-3
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,odd
    ldi rtmp,86
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 87: j -3 odd 2 (expect taken)
    ldi rarg1,-3
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,odd,taken87
    jmp after87
    taken87:
    ldi rres,1
    after87:
    ldi rtmp,87
    ldi rarg2,1
    j rres,rarg2,ne,failure

; 88: cmp 2 odd -3
    ldi rarg1,2
    ldi rarg2,-3
    cmp rres,rarg1,rarg2,odd
    ldi rtmp,88
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 89: cmpst 2 odd -3
    ldi rarg1,2
    ldi rarg2,-3
    push rarg1
    cmpst rres,rarg2,odd
    ldi rtmp,89
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 90: j 2 odd -3 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,-3
    zero rres
    j rarg1,rarg2,odd,taken90
    jmp after90
    taken90:
    ldi rres,1
    after90:
    ldi rtmp,90
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 91: cmp 2 odd 2
    ldi rarg1,2
    ldi rarg2,2
    cmp rres,rarg1,rarg2,odd
    ldi rtmp,91
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 92: cmpst 2 odd 2
    ldi rarg1,2
    ldi rarg2,2
    push rarg1
    cmpst rres,rarg2,odd
    ldi rtmp,92
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 93: j 2 odd 2 (expect fallthrough)
    ldi rarg1,2
    ldi rarg2,2
    zero rres
    j rarg1,rarg2,odd,taken93
    jmp after93
    taken93:
    ldi rres,1
    after93:
    ldi rtmp,93
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 94: cmp -2 odd -2
    ldi rarg1,-2
    ldi rarg2,-2
    cmp rres,rarg1,rarg2,odd
    ldi rtmp,94
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 95: cmpst -2 odd -2
    ldi rarg1,-2
    ldi rarg2,-2
    push rarg1
    cmpst rres,rarg2,odd
    ldi rtmp,95
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 96: j -2 odd -2 (expect fallthrough)
    ldi rarg1,-2
    ldi rarg2,-2
    zero rres
    j rarg1,rarg2,odd,taken96
    jmp after96
    taken96:
    ldi rres,1
    after96:
    ldi rtmp,96
    ldi rarg2,0
    j rres,rarg2,ne,failure

; 97: cmov 7,3,lt
    ldi rres,7
    ldi rarg1,3
    cmov rres,rarg1,lt
    ldi rtmp,97
    ldi rarg2,7
    j rres,rarg2,ne,failure

; 98: cmov -7,3,lt
    ldi rres,-7
    ldi rarg1,3
    cmov rres,rarg1,lt
    ldi rtmp,98
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 99: cmov 7,3,gt
    ldi rres,7
    ldi rarg1,3
    cmov rres,rarg1,gt
    ldi rtmp,99
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 100: cmov -7,3,gt
    ldi rres,-7
    ldi rarg1,3
    cmov rres,rarg1,gt
    ldi rtmp,100
    ldi rarg2,-7
    j rres,rarg2,ne,failure

; 101: cmov 7,7,ne
    ldi rres,7
    ldi rarg1,7
    cmov rres,rarg1,ne
    ldi rtmp,101
    ldi rarg2,7
    j rres,rarg2,ne,failure

; 102: cmov 7,3,ne
    ldi rres,7
    ldi rarg1,3
    cmov rres,rarg1,ne
    ldi rtmp,102
    ldi rarg2,3
    j rres,rarg2,ne,failure

; 103: all tested operations restore the entry stack pointer
    ld rres,[entry_sp]
    sub rres,rsp
    ldi rtmp,103
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
