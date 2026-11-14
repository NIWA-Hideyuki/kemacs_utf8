# kemacs 2.1k UTF-8対応版 日本語 MicroEMACS

kemacs は、Daniel M. Lawrence 氏が 1987 年にリリースした軽量テキストエディタ
MicroEMACS 3.8i をベースに、Takanori (sanewo) Saneto 氏による日本語 (漢字) 拡張を
施したエディタです。新出＠奈良女子大学さんの「kemacs-2.1k 非公式パッチ &
Linux対応パッチ セット (第14版)」をベースに、UTF-8 対応と最新コンパイラ
(GCC 15.2.0) 対応を重ねたものです。

## スクリーンショット

### Markdown Mode (MDMODE)

![Markdown Mode の使用法ドキュメントを編集中](docs/mdmode.png)

`docs/Markdown_mode使用法.md` を kemacs で編集しているスクリーンショット。
行頭マーカーの自動継続、番号リストの採番、アウトライン、conceal、
外部プレビューなどの機能を利用できます。

### Python Mode (PYMODE)

![Python Mode の使用法ドキュメントを編集中](docs/pymode.png)

`docs/Python_mode使用法.md` を kemacs で編集しているスクリーンショット。
Python Mode は改行時の自動インデント、`M-J` による再インデント、
`Backspace` によるデデントを提供します。

## kemacs を使うべき理由

### 1. 多言語・多エンコーディング対応

UTF-8 をはじめ、EUC-JP / Shift_JIS / JIS などの日本語エンコーディングを
シームレスに行き来できます。`iconv` を活用したラウンドトリップ変換により、
読み込みから保存の間に 1 バイトも変わらないバイト同一性が保証されます。
CJK だけでなく emoji (4 バイト UTF-8) も入力・編集可能です。

### 2. 瞬時の起動と軽量さ

C 言語で書かれた 2 万行超のコードベース。依存ライブラリは ncurses と
glibc の `iconv` のみです。ターミナルで即座に起動し、すぐに編集を
始められます。重たい IDE の立ち上げ待ちから解放されます。

### 3. 本物の Emacs キーバインド

Ctrl / Meta / Ctrl-X 二段のキースキームは今現在の Emacs とほぼそのまま
使えます。`Ctrl-X Ctrl-S` 保存、`Ctrl-X Ctrl-C` 終了、`Ctrl-S` / `Ctrl-R` の
インクリメンタルサーチ。カスタマイズ可能なキー束縛と拡張言語も備えています。

### 4. 安全な undo / redo

kemacs はコマンド境界 (command boundary) モデルで undo 履歴を管理します。
1 回の `Ctrl-X u` で 1 つのコマンドグループをまとめて取り返せ、
`Ctrl-X r` でやり直しも可能。最大保持数は `$undo-limit` で調整できます
(デフォルト 1000、0 で無制限)。

### 5. 完備されたテストスイート

kemacs は C ユニットテストフレームワークを内蔵し、17 種類のテストバイナリで
24826 件のアサーションを自動検証しています。エンコーディング変換、文字幅計算
(UAX #11 準拠)、正規表現エンジン、行操作、undo/redo、画面描画ガーター、
Python Mode / Markdown Mode / C Mode のインデントまで、すべてがテストされて
います。

## 主な機能

| 機能                 | キー / 方法                | 説明                                          |
|----------------------|----------------------------|-----------------------------------------------|
| 行番号表示           | `Ctrl-X l`                 | ガーターで行番号をトグル表示                  |
| ガーター幅の変更     | `Ctrl-X L`                 | 行番号の桁数を変更                            |
| エンコーディング変更 | `Ctrl-X A`                 | `$bcode` を設定してバッファの文字コードを切替 |
| Python Mode          | `.py` / `M-x py-mode`      | Python の自動インデント                       |
| Markdown Mode        | `.md` / `M-x md-mode`      | Markdown の構造編集                           |
| C Mode               | `.c` / `.h` / `M-x c-mode` | C の自動インデント                            |
| インクリメンタル検索 | `Ctrl-S` / `Ctrl-R`        | 正規表現対応                                  |
| バッチモード         | `-x`                       | コマンドを実行して終了                        |
| Undo                 | `Ctrl-X u`                 | 直前のコマンドグループを取り消す              |
| Redo                 | `Ctrl-X r`                 | 取り消したコマンドグループをやり直す          |

※ 行番号の `Ctrl-X l` は小文字の L、ガーター幅の `Ctrl-X L` は大文字の L です。

## ビルド

```bash
# ビルド (GCC 15 + GNU Make 必要)
make

# 起動
kemacs filename.txt

# UTF-8 で明示起動
LANG=ja_JP.UTF-8 kemacs filename.txt

# ビュー専用モード
kemacs -v filename.txt

# バイナリモード
kemacs -b filename.txt
```

## 使い方

### コマンドラインオプション

| オプション    | 説明                                  |
|---------------|---------------------------------------|
| `-v`          | ビュー専用モード (読み取り専用で開く) |
| `-e`          | 編集モード (既定)                     |
| `-b`          | バイナリモード                        |
| `-x<command>` | 起動後にコマンドを実行                |
| `-g<line>`    | 指定行へ移動                          |
| `-s<pattern>` | 起動後に検索                          |

### 基本操作

| 操作                          | キー            |
|-------------------------------|-----------------|
| 保存                          | `Ctrl-X Ctrl-S` |
| 終了                          | `Ctrl-X Ctrl-C` |
| 元に戻す                      | `Ctrl-X u`      |
| やり直す                      | `Ctrl-X r`      |
| インクリメンタル検索 (順方向) | `Ctrl-S`        |
| インクリメンタル検索 (逆方向) | `Ctrl-R`        |

## モード

### Python Mode (PYMODE)

`.py` ファイルを開くと自動で ON になります。`M-x py-mode` で手動でも
切り替えられます。

| 機能                      | コマンド           | キー        |
|---------------------------|--------------------|-------------|
| 自動インデント (改行時)   | `pyinsert`         | `Enter`     |
| 行の再インデント          | `py-indent`        | `M-J`       |
| 領域の再インデント        | `py-format-region` | `M-x`       |
| バックスペース (デデント) | `py-backspace`     | `Backspace` |

詳細は [`docs/Python_mode使用法.md`](docs/Python_mode使用法.md) を参照。

### Markdown Mode (MDMODE)

`.md` / `.markdown` ファイルを開くと自動で ON になります。
`M-x md-mode` で手動でも切り替えられます。

| 機能                    | コマンド            | キー       |
|-------------------------|---------------------|------------|
| 自動インデント (改行時) | `mdinsert`          | `Enter`    |
| 行の再インデント        | `mdindent`          | `M-J`      |
| 見出し挿入              | `md-insert-heading` | `Ctrl-X G` |
| 見出しレベル +1         | `md-heading-up`     | `Ctrl-X H` |
| アウトライン            | `md-outline`        | `M-O`      |
| 斜体                    | `md-italic`         | `M-I`      |
| conceal 表示切替        | `md-conceal-toggle` | `Ctrl-X Y` |
| 外部プレビュー          | `md-preview`        | `M-Y`      |

詳細は [`docs/Markdown_mode使用法.md`](docs/Markdown_mode使用法.md) を参照。

### C Mode (CMODE)

`.c` / `.h` ファイルを開くと自動で ON になります。`M-x c-mode` で手動でも
切り替えられます。`Enter` 時の自動インデント、`M-J` による再インデント、
リージョンの再インデントを提供します。

詳細は [`docs/cmode使用法.md`](docs/cmode使用法.md) を参照。

## 設定変数

| 変数               | デフォルト | 説明                                        |
|--------------------|------------|---------------------------------------------|
| `$bcode`           | `UTF8`     | 現在のバッファのエンコーディング            |
| `$fcode`           | `UTF8`     | 新規バッファのデフォルトエンコーディング    |
| `$undo-limit`      | `1000`     | 保持する undo レコードの最大数 (0 で無制限) |
| `$linenumbers`     | `FALSE`    | 行番号ガーターの表示/非表示                 |
| `$linenumwidth`    | `6`        | ガーター幅 (行番号の桁数 + 1 スペース)      |
| `$py-indent-width` | `4`        | Python Mode のインデント幅                  |
| `$py-use-spaces`   | `TRUE`     | Python Mode でスペースを使う                |
| `$md-indent-width` | `2`        | Markdown Mode のインデント幅                |
| `$md-auto-list`    | `TRUE`     | Markdown Mode でリストマーカーを継続        |

## 漢字コードの変更

現在のバッファのエンコーディングを変更するには、`Ctrl-X A` (set-variable) を
使用します。

```
Ctrl-X A    →   Variable to set: $bcode
              →   Value: CRLF/UTF8    (または UTF8, UJIS, SJIS 等)
```

| 設定値           | エンコーディング              |
|------------------|-------------------------------|
| `UTF8`           | UTF-8 / LF                    |
| `CRLF/UTF8`      | UTF-8 / CRLF                  |
| `CR/UTF8`        | UTF-8 / CR                    |
| `UJIS`           | EUC-JP / LF                   |
| `CRLF/UJIS`      | EUC-JP / CRLF                 |
| `SJIS`           | Shift_JIS / LF                |
| `JIS-NEW-ROMAJI` | JIS / 新シーケンス / ローマ字 |

注意: `$fcode` は新規バッファのデフォルトエンコーディングを設定するだけです。
既存バッファのエンコーディングを変更するには `$bcode` を使用してください。

## 使用例

```lisp
; .kemacsrc で永続設定
set $undo-limit 500
set $linenumbers TRUE
set $linenumwidth 4
```

## テスト

```bash
make
cd tests && make run
```

17 種類のテストバイナリ、合計 24826 件のアサーションがすべてパスします。

## プロジェクト情報

- プログラム名: `KEMACS` (漢字モード時: `日本語μEMACS`)
- バージョン: `2.1k`
- サポートエンコーディング: UTF-8 / EUC-JP / Shift_JIS / JIS
- コンパイラ: GCC 15.2.0 (ANSI C / C89 互換)

## 著作権

| 著作権者                       | モジュール                                  |
|--------------------------------|---------------------------------------------|
| Daniel M. Lawrence (1958-2010) | MicroEMACS 3.8i コアエディタ                |
| Takanori (sanewo) Saneto       | kemacs 日本語 (漢字) 拡張                   |
| Naoyuki Niide                  | kemacs 非公式パッチ + tcap.c SIGWINCH対応   |
| Dana L. Hoggatt                | crypt.c 暗号化ルーチン (既定ビルドでは無効) |
| Hideyuki Niwa                  | UTF-8 / GCC 15.2.0 互換性修正               |

## ライセンス

kemacs は非商用目的に限り自由にコピー・配布可能な元ライセンスのまま
配布されています。商用ソフトウェアへの組込みや再販は、すべての著作権者の
事前許可を得る必要があります。詳細は [`LICENSE`](LICENSE) ファイルを
参照してください。

## 関連ドキュメント

- `docs/` — 使い方・仕様書
  - `docs/kemacs使用手引書.md` — 総合手引書
  - `docs/Python_mode使用法.md` — Python Mode
  - `docs/Markdown_mode使用法.md` — Markdown Mode
  - `docs/cmode使用法.md` — C Mode
  - `docs/markdown仕様書.md` — Markdown Mode 仕様書
  - `docs/Structural_Explanation.md` — 構造解説
  - `docs/all_test_item_list.md` — テスト項目一覧
  - `docs/WSL2でCtrl+Vを使えるようにする.md` — WSL2 での Ctrl+V 設定
- `Readme` — オリジナルのリリースノート
- `emacs.key` — キー定義一覧
- `emacs.HLP_` — オンラインヘルプ
- `emacs.tut` / `emacs.TUT_` — チュートリアル
- `kemacs.man` — man ページ
- `WhatsNew` — 更新履歴
