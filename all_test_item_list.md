# `make test` — テスト項目一覧 (all_test_item_list)

最終更新: kemacs UTF-8 ブランチ

最終更新: kemacs UTF-8 ブランチ — ロケール対応ひらがな/カタカナ幅 (kana_is_wide)

`make test` は `tests/` ディレクトリのサブ Makefile を呼び出し、以下の**7つのテストプログラム**をビルドして順次実行する。
各プログラムは `test.h` フレームワークの `TEST_ASSERT*` マクロでアサーションを評価する。

## 実行結果サマリー

| テストプログラム | テスト関数数 | アサーション結果 | 対象 |
|---|---|---|---|
| test_kanji | 24 | 9012 成功 / 0 失敗 | 文字エンコーディング変換・文字幅判定 (ロケール対応 kana_is_wide) |
| test_search | 18 | 37 成功 / 0 失敗 | 検索エンジン (scanner/amatch/mceq/eq) |
| test_line | 12 | 65 成功 / 0 失敗 | バッファ行操作 (linsert/ldelete/lnewline) |
| test_undo | 13 | 73 成功 / 0 失敗 | undo/redo システム (INSERT/DELETE/NEWLINE/JOIN) |
| test_eval | 9 | 45 成功 / 0 失敗 | 式評価器ヘルパー (stoi/itoaa/gettyp/stol/ltos) |
| test_vtputc_overflow | 20 | 35 成功 / 0 失敗 | 画面描画・行あふれ('$')マーカー (80列, kana narrow/wide) |
| test_display_gutter | — | 71 成功 / 0 失敗 | ガーター描画 (行番号幅・切り詰め) |
| test_batch | 12 | 25 成功 / 0 失敗 | バッチモード統合 (kemacs -x 起動・ファイルI/O・行番号設定) |

> **合計: 89 テスト関数 / 9263 アサーション群 — 全テスト合格 (exit code 0)**

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
|| 13 | is_narrow_kanji | is_narrow_kanji |
|| 14 | is_narrow_kanji_cjk | is_narrow_kanji_cjk (CJK locale: kana as wide) |
|| 15 | iswidechar kanji | iswidechar (漢字) |
|| 16 | char_width ascii | 文字幅 (ASCII) |
|| 17 | char_width kanji | 文字幅 (漢字) |
|| 18 | char_width hankaku kana | 文字幅 (半角カタカナ) |
|| 19 | char_width unicode cjk | 文字幅 (Unicode CJK) |
|| 20 | char_width combining | 文字幅 (結合文字) |
|| 21 | char_width narrow unicode | 文字幅 (狭い Unicode) |
|| 22 | iscombchar | iscombchar (結合文字判定) |
|| 23 | char_width 8bit | 文字幅 (8ビット文字) |
|| 24 | hiragana/katakana width | ひらがな/カタカナの文字幅 |

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
|| 12 | lnewline at end | lnewline (行末での改行) |

---

## 3.5. test_undo — Undo/Redo システムユニットテスト

undo.C_ の undo/redo システムを検証する。INSERT, DELETE, NEWLINE, JOIN の各操作の undo/redo、undo 境界グループ化、undo_limit 枝刈り、u_clear_undo、および何もない場合の処理をテストする。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | undo insert single | INSERT undo (単一文字) |
| 2 | undo insert multi (boundary) | INSERT undo (複数文字・境界グループ) |
| 3 | redo insert single | INSERT redo |
| 4 | undo delete single | DELETE undo (単一文字) |
| 5 | undo delete multi (boundary) | DELETE undo (複数文字・境界グループ) |
| 6 | redo delete single | DELETE redo |
| 7 | undo newline split | NEWLINE undo (行分割の undo = 行結合) |
| 8 | redo newline split | NEWLINE redo (行結合後の再分割) |
| 9 | undo join lines | JOIN undo/redo (行結合の undo = 行再分割) |
| 10 | clear undo | u_clear_undo (バッファクリア時の undo リスト解放) |
| 11 | undo limit pruning | undo_limit 枝刈り (最大記録数保持) |
| 12 | boundary grouping | undo 境界グループ化 (u_new_bndry) |
| 13 | undo nothing to undo | undo/redo 空リスト時の "Nothing to ..." メッセージ |

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

`test_batch` は kemacs バイナリを `-x` コマンドでバッチ起動し、ファイル入出力と終了ステータスを検証する。
stdin は `/dev/null` にリダイレクトしてイベントループのハングを防止している。

| # | テスト項目 (英) | テスト項目 (日本語) |
|---|---|---|
| 1 | kemacs binary exists | kemacs バイナリの存在と読み取り可能性 |
| 2 | batch quit (clean) | `kemacs -e -xquick-exit` のクリーン終了 (exit 0) |
| 3 | batch file unchanged | ファイルを開いて `-xquick-exit` し内容が変更されないことを確認 |
| 4 | batch file modified | `-xbeginning-of-file -xquick-exit` でファイル内容が保存・保持されることを確認 |
| 5 | linenum default off | `.kemacsrc` が空の場合、行番号表示が OFF であることを確認 |
| 6 | linenum .kemacsrc set TRUE/width=8 | `.kemacsrc` に `linenumbers=8` を設定した場合の行番号幅 |
| 7 | linenum .kemacsrc set FALSE/width=3 | `.kemacsrc` に `linenumbers=3` を設定した場合の行番号幅 |
| 8 | linenum toggle on then off | `toggle-line-numbers` コマンドで ON/OFF 切替 |
| 9 | linenum toggle with arg 1 | `toggle-line-numbers 1` (引数付き) |
| 10 | linenum set-line-number-width arg 4 | `set-linenumwidth 4` |
| 11 | linenum set $linenumbers TRUE | 変数 `$linenumbers` を TRUE に設定 |
| 12 | linenum set $linenumwidth 8 | 変数 `$linenumwidth` を 8 に設定 |

---

## 6. test_vtputc_overflow — 画面描画・行あふれテスト

`vtputc()` (vtputc.C_) に対する統合テスト。画面幅80列で各種文字列を描画し、行あふれ時の `$` マーカーと文字の位置を検証する。

| # | テスト項目 (英) | テスト内容 |
|---|---|---|
| 1 | char_width | char_width の各種文字種 (ASCII, CJK, 半角カタカナ, Unicode) |
| 2 | ascii overflow | 81 ASCII 文字 → 80列目で '$' マーカー |
| 3 | wide kanji overflow | 41 漢字 → 80列目で '$' マーカー |
| 4 | wide then narrow exact | 漢字+ASCII: 40 kanji(80col) → ピッタリ収まり |
| 5 | wide then narrow overflow | 漢字+ASCII: 40 kanji(80col) + 1 → '$' |
| 6 | wide exact fit last col | 2桁表示がピッタリ80列目に収まる境界ケース |
| 7 | wide at end then overflow | 1漢字が80列目に来て、+1 → '$' |
| 8 | wide at end then overflow_wide | 1漢字が80列目、+1漢字 → '$' (前半2col未満) |
| 9 | wide start at last col | 1漢字が79列目から開始 → 2colで80/81 → '$' |
| 10 | wide with gutter overflow | ガーター(7col) + 漢字 → '$' |
| 11 | pure wide exact 80 | 40 漢字 = 80col → ピッタリ (no '$') |
| 12 | pure wide overflow 41 | 41 漢字 = 82col → '$' |
| 13 | hiragana exact fit (Western) | 80 ひらがな → 80col (1col幅, no '$') |
| 14 | hiragana overflow (Western) | 81 ひらがな → 81col → '$' (1col幅) |
| 15 | katakana overflow (Western) | 81 カタカナ → 81col → '$' (1col幅) |
| 16 | mixed ASCII + hiragana (Western) | 20 ひらがな + 61 ASCII → '$' |
| 17 | katakana with gutter (Western) | ガーター + 74 カタカナ → '$' |
| 18 | wide hiragana exact fit (CJK) | 40 ひらがな → 80col (2col幅, kana_is_wide=1) |
| 19 | wide hiragana overflow (CJK) | 41 ひらがな → '$' (2col幅) |
| 20 | wide katakana gutters (CJK) | ガーター + 37 カタカナ → '$' (2col幅) |

**重要**: テスト13-17は Western locale (`kana_is_wide = 0`) でひらがな/カタカナを1列幅として扱う。テスト18-20はCJK locale (`kana_is_wide = 1`) で2列幅として扱う。この区別なしに2列幅を仮定すると、Western terminal で画面が崩れて次の行番号が重複表示される（ユーザー報告のバグ）。

---

## 7. test_display_gutter — ガーター描画テスト

display.c のガーター (行番号欄) 描画ロジックを検証する。行番号の幅計算、文字列の切り詰め、およびガーター領域のクリアが正しく行われることを確認する。

| # | テスト項目 (英) | テスト内容 |
|---|---|---|
| — | gutter truncation | 行番号がガーター幅を超えないこと |
| — | gutter padding | ガーター領域の余白がスペースで埋められること |
| — | line number width | linenumwidth 設定によるガーター幅変更 |
| 1 | ndigits | ndigits() 関数: 1桁〜8桁の桁数計算 |
| 2 | buf_linenr | buf_linenr() 関数: キャッシュとダーティ再計算 |
| 3 | dynamic width 7-digit | 1,000,000行超バッファでガーターが7桁に自動拡張 |
| 4 | dynamic width 8-digit | 10,000,000行超バッファでガーターが8桁に自動拡張 |
| 5 | dynamic width small number | 拡張されたガーターで小さな行番号が右詰めされること |

---

## 構成 / 実行フロー (make ターゲット)

1. `make` (ルート) → `make -f make.file test`
2. `make.file` の `test:` ターゲット → `cd tests; make run`
3. `tests/Makefile` の `run:` ターゲット → `make all` でビルド後、
   `test_kanji / test_search / test_line / test_undo / test_eval /
   test_vtputc_overflow / test_display_gutter` を順次実行し、最後に `test_batch` を実行する。
   (test_batch は kemacs バイナリの存在に依存する。)

## 補足

- テストフレームワーク: `tests/test.h` (最小限のアサーションマクロ + `TEST_LIST` + `RUN_TESTS`)
- バッチテストハーネス: `tests/test_batch.c` (fork/exec で kemacs を起動)
- 手動一括実行スクリプト: `tests/run_tests.sh`
- 関連検証スクリプト (make test 本体には含まれない):
  - `repro.py` (UTF-8 絵文字ハートのエンコーディング復元確認)
  - `test_mouse_logic.py`, `test_mouse3.py` (マウスロジック検証)
  - `check_strings.py` (文字列テーブル検証)
- undo テスト用スタブ: `tests/test_line.c` に `u_record` スタブを追加 (linsert/ldelete/lnewline が undo.C_ なしでコンパイル可能にするため)
