# hanando-fukui

セルフホスト C(+いろいろ) サブセットコンパイラを作ろう: Version 3.0 (2019/8/17)

## ビルド
```
$ make
```

## コンパイル
```
$ ./hanando -f -r file.c > file.s
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

`test3.cpp` にある。
```
$ ./hanando -cpp -r -f test3.cpp > test3.s
$ gcc test3.s -o test3
$ ./test3
```
