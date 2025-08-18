# AYU\_builtin

## これは何？

伺かのAYUプロトコルを利用したシェル描画プログラムのリファレンス実装です。
ベースウェアから処理を切り離すことを目的として作られています。

現状Linux + ninix-kagariのみの対応です。

Windowsでもxdg-\*を除外してコンパイルすれば動くかもしれませんが、
現状クリックやキーボード入力などのイベントを全てスルーします。多分。

## Requirements

- OpenGL4.1に対応したPC
- C++20対応のコンパイラ
- GLFW3
- glm

## 使い方

使ってみたいゴーストの`shell/master/descript.txt`を開き、
`ayu,_builtin.exe`を追記します。
(やめたい時はこれを削除してください。)

`make`して実行ファイル`_builtin.exe`を作り、適当なディレクトリ`/path/to/ayu`に放り込みます。

ninixを次の様に呼び出します。

```
AYU_PATH="/path/to/ayu" ninix
```

## かろうじて出来ること

- サーフェスの移動(に伴うバルーンの移動)
- OnMouseClick
- ディスプレイ間の移動

## 注意

1. 時々落ちる
2. 作りが雑
3. OpenGL周りがさっぱり分かってない人の書いたコード
4. マルチスレッドのこともよく分かってない

仕様以外の部分はあまり参考にしないように。

## 仕様

[AYU(仮称)プロトコルの提案](https://gist.github.com/Tatakinov/1c88e4160f53bbd57754987c5a0e9115)

文書よりも、これの方がより最新の状況にある可能性が高いです。

## 参考にしたもの

[GLFWによるOpenGL入門](https://tokoik.github.io/GLFWdraft.pdf)
[チュートリアル14: テクスチャへのレンダー](http://www.opengl-tutorial.org/jp/intermediate-tutorials/tutorial-14-render-to-texture/)

## LICENSE

基本的にはMIT-0ですが、ソースファイルにライセンスの記述があるものはそちらが優先されます。
