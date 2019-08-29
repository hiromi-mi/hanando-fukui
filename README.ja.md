# hanando-fukui

セルフホスト C(+いろいろ) サブセットコンパイラを作ろう: Version 3.1 (2019/8/29)

## ビルド
```
$ make
```

## コンパイル
```
$ ./hanando -f file.c > file.s
$ gcc -g file.s -o file
$ ./file
```

C++ コードもどきなら `-cpp` をつける。
Objective-C 用に `-objc` も存在するが未使用。

## セルフホスト

第1世代コンパイルから第3世代コンパイラを生成し、その差分がないことを示す。
```
$ make self
$ make selfselftest
```

## template もどき (多重展開できない)

```cpp
#include <stdio.h>
template<typename T> T add(T a, T b) {
   T c;
   c = a+b;
   return c;
}

int main() {
   printf("int: %d\n", add<int>(234, 123));
   printf("char: %d\n", add<char>(234, 123));
   return 0;
}
```

このコードは `cppsamples/test3.cpp` にある。

```
$ ./hanando -cpp -f test3.cpp > test3.s
$ gcc test3.s -o test3
$ ./test3
```

すると、 char 型と int 型のサイズが異なることを利用して異なる結果になると分かる。

## class template もどき (関数宣言できない)

`cppsamples/test5.cpp` にある。 `Vector` の定義

## float/double の実装

* double 型の引数をとる箇所にint 型の引数をとるときにはキャスト必要
* !=, == 未実装
