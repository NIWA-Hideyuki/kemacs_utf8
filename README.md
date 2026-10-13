# kemacs 2.1k (UTF-8 対応版)

## 1. 概要

### 1.1 kemacs とは

kemacs (日本語 MicroEMACS) は、MicroEMACS 3.8 をベースに日本語 (KANJI) 編集機能を追加したテキストエディタです。

- プログラム名: `KEMACS` (漢字モード時: `日本語μEMACS`)
- バージョン: `2.1k`
- 複数の文字コード体系をサポート: UTF-8 / EUC-JP / Shift_JIS / JIS
- `iconv` を利用したエンコーディング変換対応


### 1.2 パッチと生成経緯

このソースは 新出＠奈良女子大学さんの「kemacs-2.1k 非公式パッチ & Linux対応パッチ セット (第14版)」をベースに
させていただきまして、AI エージェント (Hermes Agent) によって以下の修正を加えて生成されました:

1. **GCC 15.2.0 でのコンパイルエラー修正 : C99 互換性のないコードの修正
2. **UTF-8 エンコーディングのラウンドトリップ保証 :  読み込み → 保存時のバイト同一性

### 1.3 最近の GCC でのコンパイル対応

GCC 15.2.0 (またはより新しいバージョン) でコンパイルできるように、以下の修正が適用されています:

- `#include` ヘッダの明示的追加 (`stdlib.h`, `string.h` 等)
- bit-field 型の符号なし化
- `char *` と `unsigned char *` の暗黙の変換警告を抑制
- kpp (kemacs preprocessor) のプリプロセッサ出力形式対応


## 2. ビルド方法

### 必要環境

| ツール | バージョン (確認済) |
|--------|---------------------|
| GCC    | 15.2.0 (GNU C)      |
| Make   | GNU Make            |
| iconv  | glibc built-in      |


### クリーン

```bash
make clean       # 中間オブジェクトファイルのみ削除
make realclean     # すべての生成物を削除 (kpp, kanji, Cstrings も含む)
```

### ビルド

```bash
cd /home/niwan/hermes-project/kemacs_utf8/kemacs-2.1k_utf8
make
```

ビルド成功後、カレントディレクトリに `kemacs` バイナリが生成されます。
### インストール

```bash
make install 
```

## 3. 簡易な使用方法

### 3.1 起動方法

```bash
# ファイルを開く
kemacs filename.txt

# UTF-8 ファイルを明示指定して開く (デフォルトなので省略可)
TERM=xterm kemacs -fw filename.txt

# ビュー専用モードで開く
kemacs -v filename.txt

# バイナリモードで開く
kemacs -b filename.txt
```

**環境変数:**

| 変数    | 説明                                            |
|---------|-------------------------------------------------|
| `TERM`  | 端末タイプ (`xterm`, `linux`)                   |
| `HOME`  | ホームディレクトリ (`~/.kemacsrc` の検索に使用) |

**UTF-8で使うとき**

LANG=ja_JP.UTF-8

**キー操作は　M-?で詳細を参照 **

### 3.2 カーソル移動

| キー           | 機能         |
|----------------|--------------|
| `Ctrl-P` / `↑` | 前の行へ     |
| `Ctrl-N` / `↓` | 次の行へ     |
| `Ctrl-B` / `←` | 前の文字へ   |
| `Ctrl-F` / `→` | 次の文字へ   |
| `Ctrl-A`       | 行頭へ       |
| `Ctrl-E`       | 行末へ       |
| `Ctrl-X Ctrl-S`| 保存         |
| `Ctrl-X Ctrl-C`| 終了         |

### 3.3 漢字コードの変更

現在のバッファのエンコーディングを変更するには、`Ctrl-X A` (set-variable) を使用します:

```
Ctrl-X A    →   Variable to set: $bcode
              →   Value: CRLF/UTF8    (または UTF8, UJIS, SJIS 等)
```

| 設定値       | エンコーディング    |
|--------------|-------------------|
| `UTF8`       | UTF-8 / LF        |
| `CRLF/UTF8`  | UTF-8 / CRLF      |
| `CR/UTF8`    | UTF-8 / CR        |
| `UJIS`       | EUC-JP / LF       |
| `CRLF/UJIS`  | EUC-JP / CRLF     |
| `SJIS`       | Shift_JIS / LF    |
| `JIS-NEW-ROMAJI` | JIS / 新シーケンス / ローマ字 |

> **注意**: `$fcode` は新規バッファのデフォルトエンコーディングを設定するだけです。既存バッファのエンコーディングを変更するには `$bcode` を使用してください。

### 3.4 ファイルの保存

| コマンド       | キー            | 機能                        |
|----------------|-----------------|-----------------------------|
| `filesave`     | `Ctrl-X Ctrl-S` | カレントファイルに上書き保存|
| `filewrite`    | `Ctrl-X Ctrl-W` | 指定ファイル名で別名保存    |
| `exit-emacs`   | `Ctrl-X Ctrl-C` | 保存確認後終了              |

ファイルはバッファの `$bcode` で指定されたエンコーディングで保存されます。

### 3.5 Undo / Redo

kemacs は包括的な undo/redo システムを搭載しています。各コマンド実行の開始時に
undo 境界 (boundary) が設定され、1回の `undo` コマンドで1つのコマンドグループを
まとめて取り消すことができます。

| 機能 | キー | 説明 |
|------|------|------|
| `undo` | `Ctrl-X u` | 直前のコマンドグループを取り消す。繰り返し押すとさらに過去へ |
|| `redo` | `Ctrl-X r` | 取り消したコマンドグループをやり直す。繰り返し押すと未来へ |

**対象操作:** 文字の挿入 (INSERT)、文字の削除 (DELETE)、改行 (NEWLINE)、行結合 (JOIN)

**環境変数:**

| 変数 | デフォルト | 説明 |
|------|-----------|------|
| `$undo-limit` | `1000` | 保持する undo レコードの最大数。この値を超えると古いレコードが枝刈りされる |

```lisp
; .kemacsrc で undo 上限を変更
set $undo-limit 500
```

### 3.6 行番号表示 (Line Numbers)

kemacs は、テキスト領域の左端に行番号ガーターを表示する機能を搭載しています。
各行の行番号が表示されます。

**キー操作:**

| キー         | 機能                                      |
|--------------|-------------------------------------------|
| `Ctrl-X l` | 行番号表示の ON/OFF をトグル (`toggle-line-numbers`) |

**設定変数:**

| 変数            | デフォルト | 説明                                           |
|-----------------|-----------|------------------------------------------------|
| `$linenumbers`  | `FALSE`   | 行番号ガーターの表示/非表示 (ブール値)           |
| `$linenumwidth` | `6`       | ガーター幅 (行番号の桁数 + 1 スペース)            |

**`.kemacsrc` で永続的に設定:**

```lisp
; 行番号を常に表示し、幅を4桁に設定
set $linenumbers TRUE
set $linenumwidth 4
```


### 4.1 ソースコードの ANSI C (C89) 化

kemacs のソースコードは元々 K&R C および MicroEMACS 独自の拡張記法で
記述されていました。このプロジェクトでは、GCC 15 でのコンパイルと
保守性向上のため、以下の ANSI C (C89 / C90) 互換のモダン化を行っています:

- **関数定義の ANSI C 形式への変換**: K&R スタイル (`func(a, b)`) の
  関数定義を ANSI C プロトタイプ形式 (`func(int a, int b)`) に変換。
  対象: `line.c`, `eval.c`, `search.c` などのコア編集モジュール。
- **型の明示化**: `char *` / `unsigned char *` の暗黙の変換を防ぐ
  ため、型キャストや型宣言を明示的に記述。
- **ビットフィールドの符号なし化**: 構造体ビットフィールドの符号付き
  整数型を `unsigned` に統一し、コンパイラ警告を抑制。
- **ヘッダの `#include` 明示**: `stdlib.h`, `string.h` などの標準
  ヘッダを明示的に `#include` するように修正。

### 5.1 自動テスト

kemacs には `tests/` ディレクトリに自動テストスイートが配置されています。

#### テスト概要

| テストバイナリ   | 対象モジュール           | テスト内容                                   |
|------------------|--------------------------|----------------------------------------------|
| `test_kanji`     | `kanji/` (エンコーディング) | stoj, jtos, utf8_to_internal, codepoint_to_utf8, ujis_to_utf8, 文字幅計算 (iswidechar, is_narrow_kanji, char_width, kana_is_wide) |
| `test_search`    | `search.c` (正規表現)      | scanner, amatch, mceq, eq — 正規表現のマッチング、キャラクタクラス、大文字小文字無視 |
| `test_line`      | `line.c` (行操作)          | linsert, ldelete, lnewline — 行の挿入・削除・分割 |
| `test_undo`      | `undo.C_` (undo/redo)       | INSERT/DELETE/NEWLINE/JOIN undo+redo, boundary grouping, prune, clear |
| `test_eval`      | `eval.c` (式評価)          | stoi, itoaa, gettyp, stol, ltos — 文字列↔数値変換、トークン型判定 |
| `test_vtputc_overflow` | `kanji/` + `vtputc` (画面描画) | 行あふれ('$')マーカー、80列画面の幅計算 — アスキー、CJK漢字、ひらがな/カタカナ (Western: 1col, CJK: 2col) |
| `test_display_gutter` | `display.c` (ガーター描画) | 行番号ガーターの幅計算と文字列の切り詰め |
| `test_batch`     | `kemacs` バイナリ (バッチ)  | `-x` フラグによるコマンド実行、ファイル入出力検証 |

#### テストフレームワーク

`tests/test.h` に軽量な C ユニットテストフレームワークを実装しています。
外部依存はなく、標準 C のみで動作します:

- **アサーションマクロ**: `TEST_ASSERT`, `TEST_ASSERT_INT`, `TEST_ASSERT_STR`,
  `TEST_ASSERT_STRN` など
- **テスト登録**: `TEST_LIST` マクロでテスト関数を配列に登録
- **実行エンジン**: `RUN_TESTS()` で登録テストを順次実行し、
  パス/失敗数をレポート

#### テストの実行

```bash
# kemacs をビルド (make.file の生成 + 全ターゲットビルド)
make make.file
make -f make.file all

# テストをビルド + 実行 (make test ターゲット)
make test

# または tests/ ディレクトリで直接実行
cd tests
make all      # テストバイナリをビルド
make run      # テストをビルド + 実行
./run_tests.sh  # ビルド + 実行 (シェルスクリプト版)
```

`make test` は `make.file` の `test` ターゲットに委譲され、
`tests/Makefile` の `run` ターゲットを実行します。
すべてのテストがパスすると `=== All tests passed ===` が表示されます。

#### バッチモードテスト

`test_batch` は kemacs バイナリをサブプロセスとして起動し、
`-x<command>` フラグでコマンドを実行して結果を検証します。
kemacs は `-x` で指定されたコマンドを実行後、イベントループに
入るため、`quick-exit` コマンド (バッファを保存して終了) を使用して
クリーンな終了をテストします。stdin は `/dev/null` にリダイレクトされ、
イベントループのハングを防ぎます。


## 6. 関連ドキュメント

- `kemacs.man` : man ページ
- `emacs.key`  : キー定義一覧

---

## 7. ライセンス

このソフトウェアは **非商用目的に限り** 自由にコピー・配布可能な元ライセンスのまま
配布されています。商用ソフトウェアへの組込みや再販は、すべての著作権者の
事前許可を得る必要があります。

詳細は [`LICENSE`](LICENSE) ファイルを参照してください。

### 著作権者

| 著作権者 | モジュール |
|---|---|
| Daniel M. Lawrence (1958-2010) | MicroEMACS 3.8i コアエディタ |
| Dave G. Conroy, Steve Wilhite, George Jones | MicroEMACS 3.2 |
| Linwood Varney | System V / USG 対応 |
| D. R. Banks | インクリメンタルサーチ (isearch.c) |
| Takanori (sanewo) Saneto | kemacs 日本語 (漢字) 拡張 |
| Naoyuki Niide | kemacs 非公式パッチ + tcap.c SIGWINCH対応 (Linux/SysV/UTF-8) |
| Dana L. Hoggatt | crypt.c 暗号化ルーチン |
| Hideyuki Niwa | UTF-8 / GCC 15.2.0 互換性修正（Hermes Agent支援） |
