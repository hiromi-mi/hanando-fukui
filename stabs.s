gcc2_compiled.:
.stabs "/cygint/s1/users/jcm/play/",100,0,0,Ltext0
.stabs "hello.c",100,0,0,Ltext0
.text
Ltext0:
.stabs "int:t1=r1;-2147483648;2147483647;",128,0,0,0
.stabs "char:t2=r2;0;127;",128,0,0,0
.stabs "long int:t3=r1;-2147483648;2147483647;",128,0,0,0
.stabs "unsigned int:t4=r1;0;-1;",128,0,0,0
.stabs "long unsigned int:t5=r1;0;-1;",128,0,0,0
.stabs "short int:t6=r1;-32768;32767;",128,0,0,0
.stabs "long long int:t7=r1;0;-1;",128,0,0,0
.stabs "short unsigned int:t8=r1;0;65535;",128,0,0,0
.stabs "long long unsigned int:t9=r1;0;-1;",128,0,0,0
.stabs "signed char:t10=r1;-128;127;",128,0,0,0
.stabs "unsigned char:t11=r1;0;255;",128,0,0,0
.stabs "float:t12=r1;4;0;",128,0,0,0
.stabs "double:t13=r1;8;0;",128,0,0,0
.stabs "long double:t14=r1;8;0;",128,0,0,0
.stabs "void:t15=15",128,0,0,0
     .align 4
LC0:
     .ascii "Hello, world!\12\0"
     .align 4
     .global _main
     .proc 1
_main:
.stabn 68,0,4,LM1
LM1:
     !#PROLOGUE# 0
     save %sp,-136,%sp
     !#PROLOGUE# 1
     call ___main,0
     nop
.stabn 68,0,5,LM2
LM2:
LBB2:
     sethi %hi(LC0),%o1
     or %o1,%lo(LC0),%o0
     call _printf,0
     nop
.stabn 68,0,6,LM3
LM3:
LBE2:
.stabn 68,0,6,LM4
LM4:
L1:
     ret
     restore
// 関数名 N_FUN
.stabs "main:F1",36,0,0,_main
.stabn 192,0,0,LBB2
.stabn 224,0,0,LBE2
