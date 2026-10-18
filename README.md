# kemacs 2.1k UTF-8対応版 日本語 MicroEMACS

kemacs (日本語 MicroEMACS) は、Daniel M. Lawrence 氏が 1987 年にリリースした軽量テキストエディタ **MicroEMACS 3.8i** をベースに、Takanori (sanewo) Saneto 氏による日本語 (漢字) 拡張が加えられたエディタです。このソースは 新出＠奈良女子大学さんの「kemacs-2.1k 非公式パッチ & Linux対応パッチ セット (第14版)」をベースにさせていただきました。30 年以上の歳月をかけて積み重ねられた様々なパッチと、最新コンパイラ (GCC 15.2.0) 対応、そして完全 UTF-8 サポートにより、現代でもそのまま使える強力なツールになっています。

## kemacs を使うべき 5 つの理由

### 1. 多言語・多エンコーディング対応 — 4 をび演算を越える幅広い文字サポート

UTF-8 をはじめ、EUC-JP / Shift_JIS / JIS などの日本語エンコーディングをシームレスに行き来できます。`iconv` を活用したラウンドトリップ変換により、読み込み → 保存の間に **1 バイトも変わらない** バイト同一性が保証されます。CJK だけでなく emoji (4 バイト UTF-8) も入力・編集可能です。

### 2. 瞬時の起動と軽量さ — 今も昔も 100KB

C 言語で書かれた数万行のコードベース。依存ライブラリは glibc の `iconv` ほぼひとつだけ。ターミナルで即座に起動し、すぐに編集を始められます。重たい IDE の立ち上げ待ちから解放されます。

### 3. 本物の Emacs キーバインド — MicroEMACS 発祥のスタイル

Ctrl / Meta / Ctrl-X 二段のキースキームは今現在の Emacs とほぼそのまま使えます。`Ctrl-X Ctrl-S` 保存、`Ctrl-X Ctrl-C` 終了、`Ctrl-S` / `Ctrl-R` のインクリメンタルサーチ — muscle memory がそのまま生きます。カスタマイズ可能なキー束縛と拡張言語も備えています。

### 4. 安全な undo / redo — コマンド単位の境界管理

kemacs は **コマンド境界 (command boundary)** モデルで undo 履歴を管理します。1 回の `Ctrl-X u` で 1 つのコマンドグループをまとめて取り返せ、`Ctrl-X r` でやり直しも可能。最大保持数は `$undo-limit` で自由に調整でき (デフォルト 1000)、自動テストで 73 のアサーションを突破しており信頼性は高いです。

### 5. 完備されたテストスイート — 9426 のアサーションがすべてパス

kemacs は C ユニットテストフレームワークを内蔵し、8 種類のテストバイナリで **9426 件のアサーション** を自動検証中です。エンコーディング変換、文字幅計算 (UAX #11 準拠)、正規表現エンジン、行操作、undo/redo、画面描画ガーターまで — すべてがテストされています。

## 機能一覧

| 機能 | キー | 説明 |
|------|------|------|
| 行番号表示 | `Ctrl-X l` | ガーターで行番号トグル。`$linenumbers` / `$linenumwidth` で永続設定 |
| バッファエンコーディング変更 | `Ctrl-X A` | `$bcode` で UTF-8 / EUC-JP / SJIS / JIS 自由切替 |
| ファイル暗号化 | `-k` | crypt.c (Dana L. Hoggatt 著作) による暗号化 |
| 拡張検索 | `Ctrl-S` / `Ctrl-R` | 正規表現 + インクリメンタルサーチ |
| バッチモード | `-x` | コマンドを指定して対話なしで実行可能 |
| Undo | `Ctrl-X u` | 直前のコマンドグループを取り消す |
| Redo | `Ctrl-X r` | 取り消したコマンドグループをやり直す |

## はじめかた

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

## 設定変数

| 変数 | デフォルト | 説明 |
|------|------------|------|
| `$bcode` | `UTF8` | 現在のバッファのエンコーディング |
| `$fcode` | `UTF8` | 新規バッファのデフォルトエンコーディング |
| `$undo-limit` | `1000` | 保持する undo レコードの最大数 |
| `$linenumbers` | `FALSE` | 行番号ガーターの表示/非表示 |
| `$linenumwidth` | `6` | ガーター幅 (行番号の桁数 + 1 スペース) |

## 漢字コードの変更

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

## 使用例

```lisp
; .kemacsrc で永続設定
set $undo-limit 500
set $linenumbers TRUE
set $linenumwidth 4
```

## プロジェクト情報

- **プログラム名**: `KEMACS` (漢字モード時: `日本語μEMACS`)
- **バージョン**: `2.1k`
- **サポートエンコーディング**: UTF-8 / EUC-JP / Shift_JIS / JIS
- **コンパイラ**: GCC 15.2.0 (ANSI C / C89 互換)
- **テスト**: 8 種類のテストバイナリ、合計 9426 アサーション — すべてパス

## 著作権者

| 著作権者 | モジュール |
|----------|-----------|
| Daniel M. Lawrence (1958-2010) | MicroEMACS 3.8i コアエディタ |
| Takanori (sanewo) Saneto | kemacs 日本語 (漢字) 拡張 |
| Naoyuki Niide | kemacs 非公式パッチ + tcap.c SIGWINCH対応 |
| Dana L. Hoggatt | crypt.c 暗号化ルーチン |
| Hideyuki Niwa | UTF-8 / GCC 15.2.0 互換性修正 (Hermes Agent支援) |

## ライセンス

kemacs は **非商用目的に限り** 自由にコピー・配布可能な元ライセンスのまま配布されています。商用ソフトウェアへの組込みや再販は、すべての著作権者の事前許可を得る必要があります。詳細は [`LICENSE`](LICENSE) ファイルを参照してください。

## 関連ドキュメント

- `README.md` — 詳しい使い方とビルド方法
- `README` — オリジナルの日本語説明書
- `emacs.key` — キー定義一覧
- `emacs.HLP_` — オンラインヘルプ
- `emacs.tut` / `emacs.TUT_` — チュートリアル
- `kemacs.man` — man ページ
- `WhatsNew` — 更新履歴

---

> **注意**: kemacs は MicroEMACS 3.8i の派生作品です。商用利用には著作権者の許可が必要です。
