#include <stdio.h>

int main() {
   int x;
   x = 8;
   return x;
}

// stabs: 文字列
// stabn: 数値
// stabdd (dot) ?
//
// .stabs "string",type,other,desc,value
// .stabn type,other,desc,value
// .stabd type,other,desc
// value フィールドは自明
// type フィールド: stab のどの型が普通なのか
// 2.3 は読み飛ばす (Include Files の名前は不要)
//
/*

   .stabs "1.c",4,0,0,Ltext0
   // 0x4 がC言語らしい、100 かも？
   .text
Ltext0:
.stabs "int:t1=r1;-21...;",128,0,0,0
// 0x44: N_SLINE が行番号の最初らしい
idesc = 行番号
other = 0
value = コードアドレス (参照を表す)
.stabn 0x44,0,行番号, LM5

// 関数名 N_FUN 0x24
.stabs "main:F1",36,0,0,_main
// f は static F はglobal
// 再配置ごにょぎょは後回し？
.stabs "main:f1",36,0,0,_main


// 2.6 ネストしてると comma で繋ぐ baz:f1,baz,bar,... と徐々に外側に
// 2.7 
// Begin of a lexical block 192 N_LBRAC
.stabn 192,0,0,LBE2
// End of a lexical block 0xe0 N_RBRAC
.stabn 224,0,0,LBE2

// 4. 変数定義
// スタック上に確保される自動変数 N_LSYM シンボル 4.1節
// 第四引数value は ローカル変数内でのoffset: frame pointer で大体負
.stabs "x:1", 128,0,0,-2
// N_GSYM (32) 4.2節
.stabs "g_foo:G2", 32, 0, 0, 0 (G シンボルディスクー)
// N_PSYM 4.6節 (パラメータ)
.stabs "x:p13", 160,0,3,8
desc は行番号

// 5. Builtin Type 
*/
