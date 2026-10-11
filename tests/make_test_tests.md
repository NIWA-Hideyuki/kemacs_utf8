# `make test` テスト項目一覧

最終更新: kemacs UTF-8 ブランチ
実行方法: プロジェクトルートで `make test`
結果: 全テスト合格 (exit code 0)

`make test` は `tests/` ディレクトリのサブ Makefile を呼び出し、
以下の5つのテストプログラムをビルドして順次実行する。
各プログラムは `test.h` フレームワークの `TEST_ASSERT*` マクロで
アサーションを評価する。

---

## 1. test_kanji  (漢字/文字処理ユニットテスト)

文字エンコーディング変換と文字幅判定を検証する。
実行時のアサーション結果: 9007 件成功 / 0 件失敗 (23 テスト関数)

- [stoj/jtos known pairs]           stoj/jtos の既知ペア変換
- [stoj/jtos round-trip]            stoj → jtos のラウンドトリップ
- [stoj invalid input]              stoj に無効入力
- [jtos invalid input]              jtos に無効入力
- [utf8_to_codepoint]               UTF-8 → Unicode コードポイント
- [utf8_to_internal ascii]          UTF-8 → 内部表現 (ASCII)
- [utf8_to_internal invalid]        UTF-8 → 内部表現 (無効入力)
- [codepoint_to_utf8]               Unicode コードポイント → UTF-8
- [ujis_to_utf8 ascii]              UJIS → UTF-8 (ASCII 範囲)
- [ujis_to_utf8 fullwidth]          UJIS → UTF-8 (全角スペース)
- [iswidechar ascii]                iswidechar (ASCII)
- [iswidechar kana]                 iswidechar (ひらがな/カタカナ)
- [is_narrow_kanji]                 is_narrow_kanji
- [iswidechar kanji]                iswidechar (漢字)
- [char_width ascii]                文字幅 (ASCII)
- [char_width kanji]                文字幅 (漢字)
- [char_width hankaku kana]         文字幅 (半角カタカナ)
- [char_width unicode cjk]          文字幅 (Unicode CJK)
- [char_width combining]            文字幅 (結合文字)
- [char_width narrow unicode]       文字幅 (狭い Unicode)
- [iscombchar]                      iscombchar (結合文字判定)
- [char_width 8bit]                 文字幅 (8ビット文字)
- [hiragana/katakana width]         ひらがな/カタカナの文字幅

---

## 2. test_search  (検索/パターンマッチユニットテスト)

検索エンジン (scanner, amatch, mceq, eq) の一致ロジックを検証する。
実行時のアサーション結果: 37 件成功 / 0 件失敗 (18 テスト関数)

- [scanner found]                   scanner の基本一致
- [scanner found (PTEND)]           scanner の一致 (PTEND 境界)
- [scanner not found]               scanner の非一致
- [scanner empty pattern]           scanner に空パターン
- [scanner at start]                scanner の先頭一致
- [scanner single char]             scanner の1文字一致
- [amatch match]                    amatch の基本一致
- [amatch fail]                     amatch の非一致
- [amatch with ANY]                 amatch の ANY ワイルドカード
- [amatch with BOL]                 amatch の BOL (行頭) アンカー
- [amatch with EOL]                 amatch の EOL (行末) アンカー
- [amatch partial fail]             amatch の部分一致失敗
- [mceq LITCHAR]                    mceq のリテラル文字
- [mceq ANY]                        mceq の ANY ワイルドカード
- [mceq case exact]                 mceq の大文字小文字を区別
- [mceq case fold]                  mceq の大文字小文字を無視
- [eq exact]                        eq の完全一致
- [eq casefold]                     eq の大文字小文字を無視

---

## 3. test_line  (バッファ行操作ユニットテスト)

行操作 (linsert, ldelete, lnewline) のテキスト挿入/削除/分割を検証する。
実行時のアサーション結果: 65 件成功 / 0 件失敗 (12 テスト関数)

- [linsert middle]                  linsert (行中への挿入)
- [linsert at end]                  linsert (行末への挿入)
- [linsert empty line]              linsert (空行への挿入)
- [linsert one at start]            linsert (行頭1文字の挿入)
- [ldelete simple]                  ldelete (単純削除)
- [ldelete to end]                  ldelete (行末まで削除)
- [ldelete at end]                  ldelete (行末での削除)
- [ldelete zero]                    ldelete (0文字削除)
- [ldelete all]                     ldelete (全削除)
- [lnewline split]                  lnewline (行分割)
- [lnewline at start]               lnewline (行頭での改行)
- [lnewline at end]                 lnewline (行末での改行)

---

## 4. test_eval  (式評価器ヘルパーテスト)

eval.c のヘルパー (stoi, itoaa, gettyp, stol, ltos) を検証する。
eval.c は -ffunction-sections + --gc-sections で必要な関数のみリンクされる。
実行時のアサーション結果: 45 件成功 / 0 件失敗 (9 テスト関数)

- [stoi_basic]                      stoi: 基本整数変換 (42, 0, -1, -100)
- [stoi_edge]                       stoi: 境界ケース (先頭空白、非数字停止)
- [itoaa_basic]                     itoaa: 基本整数→文字列
- [itoaa_negative]                  itoaa: 負の整数
- [gettyp_literals]                 gettyp: リテラル型判定
- [gettyp_quoted]                   gettyp: クォート文字列型判定
- [gettyp_prefixes]                 gettyp: プレフィックス型判定
- [stol]                            stol: 長整数変換
- [ltos]                            ltos: 長整数→文字列

---

## 5. test_batch  (バッチモード統合テスト)

kemacs バイナリを `-x` コマンドでバッチ起動し、ファイル入出力と終了
ステータスを検証する。stdin は /dev/null にリダイレクトしてイベントループ
のハングを防止している。
実行時のアサーション結果: 8 件成功 / 0 件失敗 (4 テスト関数)

- [kemacs binary exists]            kemacs バイナリの存在と読み取り可能性
- [batch quit (clean)]              `kemacs -e -xquick-exit` のクリーン終了 (exit 0)
- [batch file unchanged]            ファイルを開いて `-xquick-exit` し内容が変更されないことを確認
- [batch file modified]             `-xbeginning-of-file -xquick-exit` でファイル内容が保存・保持されることを確認

---

## 構成 / 実行フロー (make ターゲット)

1. `make` (ルート) → `make -f make.file test`
2. `make.file` の `test:` ターゲット → `cd tests; make run`
3. `tests/Makefile` の `run:` ターゲット → `make all` でビルド後、
   test_kanji / test_search / test_line / test_eval を順次実行し、
   最後に test_batch を実行する。
   （test_batch は kemacs バイナリの存在に依存する。）

## 補足

- テストフレームワーク: `tests/test.h` (最小限のアサーションマクロ + TEST_LIST + RUN_TESTS)
- バッチテストハーネス: `tests/test_batch.c` (fork/exec で kemacs を起動)
- 手動一括実行スクリプト: `tests/run_tests.sh`
- 関連検証スクリプト (make test 本体には含まれない):
  - `repro.py` (UTF-8 絵文字ハートのエンコーディング復元確認)
  - `test_mouse_logic.py`, `test_mouse3.py` (マウスロジック検証)
  - `check_strings.py` (文字列テーブル検証)
