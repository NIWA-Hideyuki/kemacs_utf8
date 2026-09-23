# kemacs 2.1k (UTF-8 対応版)

## 1. 概要

### 1.1 kemacs とは

kemacs (日本語 MicroEMACS) は、MicroEMACS 3.8 をベースに日本語 (KANJI) 編集機能を追加したテキストエディタです。

- プログラム名: `KEMACS` (漢字モード時: `日本語μEMACS`)
- バージョン: `2.1k`
- 複数の文字コード体系をサポート: UTF-8 / EUC-JP / Shift_JIS / JIS
- `iconv` を利用したエンコーディング変換対応
- 8ビットスルーモード (バイナリ編集) 対応

### 1.2 パッチと生成経緯

このソースは 新出＠奈良女子大学さんの「kemacs-2.1k 非公式パッチ & Linux対応パッチ セット (第14版)」をベースに
させていただきまして、AI エージェント (Hermes Agent) によって以下の修正を加えて生成されました:

1. **GCC 15.2.0 でのコンパイルエラー修正 : C99 互換性のないコードの修正
2. **UTF-8 エンコーディングのラウンドトリップ保証 :  読み込み → 保存時のバイト同一性
3. **モードライン表示バグ修正 : 漢字コード表示の1文字残留問題 (μ 文字の表示幅修正)

### 1.3 最近の GCC でのコンパイル対応

GCC 15.2.0 (またはより新しいバージョン) でコンパイルできるように、以下の修正が適用されています:

- `#include` ヘッダの明示的追加 (`stdlib.h`, `string.h` 等)
- bit-field 型の符号なし化
- `char *` と `unsigned char *` の暗黙の変換警告を抑制
- kpp (kemacs preprocessor) のプリプロセッサ出力形式対応

### 1.4 UTF-8 のサポート

| 設定項目     |     値    | 説明                                           |
|--------------|-----------|------------------------------------------------|
| `DEF_F_CODE` | `KS_UTF8` | デフォルトファイルエンコーディング (econfig.h) |
| `DEF_F_EOL`  | `KS_CRLF` | デフォルトファイル行末コード                   |
| `DEF_T_CODE` | `KS_UTF8` | デフォルト端末エンコーディング                 |
| `HANDLE_UTF` |     1     | UTF-8 変換機能 (iconv) 有効                    |

---

## 2. ビルド方法

### 必要環境

| ツール | バージョン (確認済) |
|--------|---------------------|
| GCC    | 15.2.0 (GNU C)      |
| Make   | GNU Make            |
| iconv  | glibc built-in      |

### ビルド

```bash
cd /home/niwan/hermes-project/kemacs_utf8/kemacs-2.1k_utf8
make
```

ビルド成功後、カレントディレクトリに `kemacs` バイナリが生成されます。

### クリーンビルド

```bash
make clean       # 中間オブジェクトファイルのみ削除
make realclean     # すべての生成物を削除 (kpp, kanji, Cstrings も含む)
```

### インストール

```bash
make install 
```

---

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

---

## 4. 関連ドキュメント

- `kemacs.man` : man ページ
- `emacs.key`  : キー定義一覧

---

## 5. ライセンス

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
