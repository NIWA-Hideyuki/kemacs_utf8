# `make test` — テスト項目一覧 (all_test_item_list)

最終更新: kemacs UTF-8 ブランチ

`make test` は `tests/` ディレクトリのサブ Makefile を呼び出し、以下の5つのテストプログラムをビルドして順次実行する。
各プログラムは `test.h` フレームワークの `TEST_ASSERT*` マクロでアサーションを評価する。

## 実行結果サマリー

| テストプログラム | テスト関数数 | アサーション結果 | 対象 |
|---|---|---|---|
| test_kanji | 23 | 9007 成功 / 0 失敗 | 文字エンコーディング変換・文字幅判定 |
| test_search | 18 | 37 成功 / 0 失敗 | 検索エンジン (scanner/amatch/mceq/eq) |
| test_line | 12 | 65 成功 / 0 失敗 | バッファ行操作 (linsert/ldelete/lnewline) |
| test_eval | 9 | 45 成功 / 0 失敗 | 式評価器ヘルパー (stoi/itoaa/gettyp/stol/ltos) |
| test_batch | 4 | 8 成功 / 0 失敗 | バッチモード統合 (kemacs -x 起動・ファイルI/O) |

> **合計: 66 テスト関数 / 695 アサーション群 — 全テスト合格 (exit code 0)**
>
> test_kanji の「9007 アサーション」は23関数のうちの1つのアサーションを反復回した合計である。
> 各関数が実行する個別アサーション数は test.h の `test_count` でカウントされる。

---

## 1. test_kanji — 漢字/文字処理ユニットテスト

文字エンコーディング変換と文字幅判定を検証する。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | stoj/jtos known pairs | stoj/jtos の既知ペア変換 |
| 2 | stoj/jtos round-trip | stoj → jtos のラウンドトリップ |
| 3 | stoj invalid input | stoj に無効入力 |
| 4 | jtos invalid input | jtos に無効入力 |
| 5 | utf8_to_codepoint | UTF-8 → Unicode コードポイント |
| 6 | utf8_to_internal ascii | UTF-8 → 内部表現 (ASCII) |
| 7 | utf8_to_internal invalid | UTF-8 → 内部表現 (無効入力) |
| 8 | codepoint_to_utf8 | Unicode コードポイント → UTF-8 |
| 9 | ujis_to_utf8 ascii | UJIS → UTF-8 (ASCII 範囲) |
| 10 | ujis_to_utf8 fullwidth | UJIS → UTF-8 (全角スペース) |
| 11 | iswidechar ascii | iswidechar (ASCII) |
| 12 | iswidechar kana | iswidechar (ひらがな/カタカナ) |
| 13 | is_narrow_kanji | is_narrow_kanji |
| 14 | iswidechar kanji | iswidechar (漢字) |
| 15 | char_width ascii | 文字幅 (ASCII) |
| 16 | char_width kanji | 文字幅 (漢字) |
| 17 | char_width hankaku kana | 文字幅 (半角カタカナ) |
| 18 | char_width unicode cjk | 文字幅 (Unicode CJK) |
| 19 | char_width combining | 文字幅 (結合文字) |
| 20 | char_width narrow unicode | 文字幅 (狭い Unicode) |
| 21 | iscombchar | iscombchar (結合文字判定) |
| 22 | char_width 8bit | 文字幅 (8ビット文字) |
| 23 | hiragana/katakana width | ひらがな/カタカナの文字幅 |

---

## 2. test_search — 検索/パターンマッチユニットテスト

検索エンジン (scanner, amatch, mceq, eq) の一致ロジックを検証する。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | scanner found | scanner の基本一致 |
| 2 | scanner found (PTEND) | scanner の一致 (PTEND 境界) |
| 3 | scanner not found | scanner の非一致 |
| 4 | scanner empty pattern | scanner に空パターン |
| 5 | scanner at start | scanner の先頭一致 |
| 6 | scanner single char | scanner の1文字一致 |
| 7 | amatch match | amatch の基本一致 |
| 8 | amatch fail | amatch の非一致 |
| 9 | amatch with ANY | amatch の ANY ワイルドカード |
| 10 | amatch with BOL | amatch の BOL (行頭) アンカー |
| 11 | amatch with EOL | amatch の EOL (行末) アンカー |
| 12 | amatch partial fail | amatch の部分一致失敗 |
| 13 | mceq LITCHAR | mceq のリテラル文字 |
| 14 | mceq ANY | mceq の ANY ワイルドカード |
| 15 | mceq case exact | mceq の大文字小文字を区別 |
| 16 | mceq case fold | mceq の大文字小文字を無視 |
| 17 | eq exact | eq の完全一致 |
| 18 | eq casefold | eq の大文字小文字を無視 |

---

## 3. test_line — バッファ行操作ユニットテスト

行操作 (linsert, ldelete, lnewline) のテキスト挿入/削除/分割を検証する。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | linsert middle | linsert (行中への挿入) |
| 2 | linsert at end | linsert (行末への挿入) |
| 3 | linsert empty line | linsert (空行への挿入) |
| 4 | linsert one at start | linsert (行頭1文字の挿入) |
| 5 | ldelete simple | ldelete (単純削除) |
| 6 | ldelete to end | ldelete (行末まで削除) |
| 7 | ldelete at end | ldelete (行末での削除) |
| 8 | ldelete zero | ldelete (0文字削除) |
| 9 | ldelete all | ldelete (全削除) |
| 10 | lnewline split | lnewline (行分割) |
| 11 | lnewline at start | lnewline (行頭での改行) |
| 12 | lnewline at end | lnewline (行末での改行) |

---

## 4. test_eval — 式評価器ヘルパーテスト

eval.c のヘルパー (stoi, itoaa, gettyp, stol, ltos) を検証する。
eval.c は `-ffunction-sections` + `--gc-sections` で必要な関数のみリンクされる。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | stoi_basic | stoi: 基本整数変換 (42, 0, -1, -100) |
| 2 | stoi_edge | stoi: 境界ケース (先頭空白、非数字停止) |
| 3 | itoaa_basic | itoaa: 基本整数→文字列 |
| 4 | itoaa_negative | itoaa: 負の整数 |
| 5 | gettyp_literals | gettyp: リテラル型判定 |
| 6 | gettyp_quoted | gettyp: クォート文字列型判定 |
| 7 | gettyp_prefixes | gettyp: プレフィックス型判定 |
| 8 | stol | stol: 長整数変換 |
| 9 | ltos | ltos: 長整数→文字列 |

---

## 5. test_batch — バッチモード統合テスト

kemacs バイナリを `-x` コマンドでバッチ起動し、ファイル入出力と終了ステータスを検証する。
stdin は `/dev/null` にリダイレクトしてイベントループのハングを防止している。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | kemacs binary exists | kemacs バイナリの存在と読み取り可能性 |
| 2 | batch quit (clean) | `kemacs -e -xquick-exit` のクリーン終了 (exit 0) |
| 3 | batch file unchanged | ファイルを開いて `-xquick-exit` し内容が変更されないことを確認 |
| 4 | batch file modified | `-xbeginning-of-file -xquick-exit` でファイル内容が保存・保持されることを確認 |

---

## 構成 / 実行フロー (make ターゲット)

1. `make` (ルート) → `make -f make.file test`
2. `make.file` の `test:` ターゲット → `cd tests; make run`
3. `tests/Makefile` の `run:` ターゲット → `make all` でビルド後、
   `test_kanji / test_search / test_line / test_eval` を順次実行し、最後に `test_batch` を実行する。
   (test_batch は kemacs バイナリの存在に依存する。)

## 補足

- テストフレームワーク: `tests/test.h` (最小限のアサーションマクロ + `TEST_LIST` + `RUN_TESTS`)
- バッチテストハーネス: `tests/test_batch.c` (fork/exec で kemacs を起動)
- 手動一括実行スクリプト: `tests/run_tests.sh`
- 関連検証スクリプト (make test 本体には含まれない):
  - `repro.py` (UTF-8 絵文字ハートのエンコーディング復元確認)
  - `test_mouse_logic.py`, `test_mouse3.py` (マウスロジック検証)
  - `check_strings.py` (文字列テーブル検証)
