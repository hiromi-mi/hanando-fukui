.intel_syntax noprefix
.align 4
.file 1 "1.c"
.data
.stabs "1.c",100,0,0,.Ltext0
.stabs "int:t1=r1;-2147483648;2147483647;",128,0,0,0
   // 0x4 がC言語らしい、100 かも？
.Ltext0:
.text
.type main,@function
.global main
.stabs "main:F1",36,0,0,.main
// Begin of a lexical block 192 N_LBRAC
.main:
main:
push rbp
mov rbp, rsp
sub rsp, 8
.LBB2:
.LM5:
.loc 1 5
mov r10d, 8
mov dword ptr [rbp-8], r10d
.loc 1 6
.LM6:
mov eax, dword ptr [rbp-8]
mov rsp, rbp
pop rbp
.LBE2:
ret
// End of a lexical block 0xe0 N_RBRAC
.stabn 192,0,0,.LBB2
.stabn 224,0,0,.LBE2
.stabn 0x44,0,5, .LM5
.stabn 0x44,0,6, .LM6
.stabs "x:1", 128,0,0,-8
