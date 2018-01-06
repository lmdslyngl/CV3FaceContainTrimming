
# CV3FaceContainTrimming

## なにこれ？
* なるべく顔が写るようにスクリーンショットを正方形に切り抜くプログラムです。
* スクリーンショットのサムネイル生成などに使えます。

## 使い方
* 最も基本的な使い方
    * 入力画像のうち顔が一番写っていそうな領域を正方形で切り抜いて、出力画像に書き出します。

```
CV3FaceContainTrimming.exe --input {入力画像パス} --output {出力画像パス}
```

* また、--outputに"#"を指定することで、検出された顔領域を可視化することができます。
    * 詳細オプション引数を使う際にデバッグかなにかに使えるかも知れませんね。

### 詳細オプション引数
「分かっている人」向けのオプションです。
通常はこれらのオプションを使用する必要はありません。

* --cascade-xml, -x
    * OpenCVで使用するCascadeClassifierのモデルファイルを指定します
* --cascade-scale-factor, -s
    * CascadeClassifier.detectMultiScaleで使用する値です
* --cascade-neighbor, -n
    * CascadeClassifier.detectMultiScaleで使用する値です
* --cascade-min-ratio, -m
    * CascadeClassifier.detectMultiScaleで使用する値です

## よくありそうな質問

### ゲームのスクリーンショットにしか使えないの？
--cascade-xmlオプションを用いてモデルファイルを変更することで、実写の写真でも対応できます。

### 誤検出多すぎ
ボクもそう思います。使用しているモデルの都合でこれ以上の精度は見込めません。

### 正方形以外に切り抜きたい
ボクもそう思います。そのうち実装します。

