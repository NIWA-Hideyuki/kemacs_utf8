# kemacs CMODE 使用方法

kemacs に組み込まれた **CMODE (`MDCMOD`)** は、C 言語 (C89/C90/C99/C11/C17/C23) の編集において、インデントと入力補助を自動的に行う機能です。C の構文を解析して適切なインデントを計算します。

---

## 1. 機能概要

| 機能 | コマンド名 | キー | 説明 |
|------|-----------|------|------|
| 自動インデント (改行時) | `cinsert` | `Enter` | 前の行のブレース・括弧・キーワードに基づいて新しい行のインデントを自動計算 |
| 行の再インデント | `cindent` | `M-J` | カレント行のインデントを前行の文脈に基づいて修正 |
| 領域の再インデント | `cindent-region` | `M-x` | 選択リージョン内の各行を C 構文に基づいて再インデント |
| スタイル再検出 | `c-detect-style` | `M-x` | ファイル内の既存コードからブレーススタイル・インデント単位を自動検出 |
| モード切り替え | `c-mode` | `M-x` | CMODE の ON/OFF を切り替える |

---

## 2. CMODE の有効化

CMODE は **バッファローカル** なモードです。以下の方法で有効にできます。

### 2.1 ファイル拡張子による自動判定

`.c` / `.h` 拡張子のファイルを開くと、`AUTOMODE` が有効な場合に自動的に CMODE がONになります。

ファイルを開く際、`$c-auto-detect` が TRUE に設定されている場合、CMODE はファイル内の既存コードから以下を自動検出します:

1. **インデント単位**: タブ vs スペース, 2/4/8 スペース
2. **ブレーススタイル**: K&R, Allman, GNU, Whitesmiths, 1TBS, Stroustrup のいずれか
3. **case ラベルのインデント**: switch と同じレベルか、+1 レベルか

### 2.2 手動で有効化

```
M-x c-mode    (c-mode と入力)
```

`M-x` は「名前を指定してコマンドを実行する」コマンドです。`c-mode` と入力すると、カレントバッファの CMODE (`MDCMOD` ビット) が ON/OFF 切り替えられます。

引数を指定することもできます:

- `M-x c-mode` (引数なし): ON/OFF トグル
- `Ctrl \[` で数値引数 `1` を指定して `M-x c-mode`: CMODE を強制的にON
- 数値引数 `-1`: CMODE を強制的にOFF

### 2.3 スタイルの自動検出

ファイルを開いた後、またはいつでも以下のコマンドでスタイルを再検出できます:

```
M-x c-detect-style
```

このコマンドは、ファイルの最初の 200 行をスキャンし、以下のアルゴリズムでブレーススタイルとインデント単位を推測します:

1. タブのみのリーダー → `c_use_spaces = FALSE` (タブモード)
2. スペースのみのリーダー → `c_use_spaces = TRUE`, 最も頻出するリーダー長から `c_indent_width` を決定 (2/4/8)
3. タブ + スペース混在 → Google スタイル (タブでインデント, スペースでアライン)
4. ブレーススタイルを検出: `if (...) {` が多い→K&R, `{` が独立行→Allman, +2 カラム→GNU, 本文と同じ→Whitesmiths
5. 検出不能な場合 → スタイル 7 (none), タブモード, `c_indent_width = 8`

---

## 3. キー操作

### 3.1 `Enter` (cinsert) -- 自動インデント

CMODE がONのバッファで `Enter` を押すと、前の行の内容に基づいて新しい行のインデントが自動計算されます。

**ルール:**

| 前の行の末尾 | アクション |
|-------------|-----------|
| `{` | 1 レベル深くインデント (c-indent-width スペースまたは 1 タブ) |
| `}` | 対応する `{` のインデントレベルに戻る (c-auto-close-brace が TRUE 時) |
| `(` / `[` (未閉) | 継続行インデント (c-cont-indent 追加) |
| `if` / `for` / `while` 等のキーワード (ブレースなし) | 1 レベル深くインデント |
| `else` / `case` / `default` | 対応する `if` / `switch` と同じレベル (case は +c-case-indent) |
| `#if` / `#ifdef` / `#ifndef` | 1 レベル深くインデント (c-pp-indent 適用) |
| `#else` / `#elif` | `#if` と同じレベル |
| `#endif` | `#if` のレベルに戻る |
| `do` | `do` と同じレベル (do-while の `}` は `do` に揃える) |
| `#` (その他のプリプロセッサ) | c-pp-indent スペース |
| 上記以外 | 前の行と同じインデント |

```
if (x) {          ──Enter──▶
	do_something();      │
                    ▼    ← 1 レベル自動インデント (タブ)

for (int i = 0;   ──Enter──▶
     i < n;       │  ← 継続行インデント (c-cont-indent)
     i++) {       │
	process(i);   │
                ▼    ← 1 レベルインデント

}               ──Enter──▶
            ▼    ← } は if と同じレベル (0 列)
```

### 3.2 `M-J` (cindent) -- 行の再インデント

カーソル行のインデントを、前の非空行の文脈に基づいて再計算します。

```
	if (x) {        ──M-J──▶
	do_something();          │
                         ▼  ← インデント修正
	do_something();
```

### 3.3 `M-x cindent-region` (cindent-region) -- 領域の再インデント

リージョン (範囲選択) 内の各行を、C のブロック構造に基づいて再インデントします。インデントスタックを使用して、入れ子構造を正しく解析します。

**使用方法:**
1. `M-space` でマークを設定
2. カーソルを移動して再インデントしたい範囲を選択
3. `M-x` を押して `cindent-region` と入力

### 3.4 `}` (insbrace) -- 閉じブレース入力補助

CMODE がONのバッファで `}` を入力すると:
1. タブストップまでバックスペース (元のロジック)
2. `c-auto-close-brace` が TRUE の場合: 対応する行のインデントを再計算

### 3.5 `#` (inspound) -- プリプロセッサ `#` 入力補助

CMODE がONのバッファで `#` を入力すると:
1. 行頭空白をクリーンアップ (元のロジック)
2. `c-pp-indent` が 0 以外の場合: 指定されたインデントで `#` を配置

---

## 4. 設定変数 (Evar)

CMODE の動作は以下の 11 つのエディタ変数 (`$c-*`) で制御します。

### 4.1 `$c-indent-width` (デフォルト: 4)

1 レベルのインデント幅を指定します。スペースモード (`c-use-spaces = TRUE`) のときはスペース数、タブモード (`c-use-spaces = FALSE`) のときはタブ幅の基準となります。

```
set $c-indent-width 2    (2 スペース/タブインデント)
set $c-indent-width 8    (8 スペース/タブインデント)
```

### 4.2 `$c-use-spaces` (デフォルト: FALSE)  -- タブがデフォルト

インデントにスペースを使用するかどうかを BOOL 値で指定します。

- `FALSE` (デフォルト): **タブ**を使用 (タブ幅は `$tabsize` または `tabsize` グローバル変数に依存)
- `TRUE`: スペースを使用

```
set $c-use-spaces FALSE   (タブモード -- デフォルト)
set $c-use-spaces TRUE    (スペースモード)
```

### 4.3 `$c-brace-style` (デフォルト: 0)

ブレーススタイルを指定します。`}` (閉じ括弧) のインデントターゲットと構造的インデントの解釈に影響を与えます。

| 値 | スタイル名 | 説明 | `}` のインデント |
|---|---|---|---|
| 0 | K&R | `{` は行末, `}` は独立行で制御文と同じレベル | 制御文レベル |
| 1 | Allman | `{` と `}` は独立行で制御文と同じレベル | 制御文レベル |
| 2 | GNU | `{` と `}` は独立行で半分インデント (2 カラム) | 制御文レベル + 2 |
| 3 | Linux | K&R と同じ (Linux カーネルスタイル, タブ使用) | 制御文レベル |
| 4 | Whitesmiths | `{` は独立行で本文と同じレベル | 本文レベル |
| 5 | 1TBS | K&R に近似, `else`/`}` を前の行にくっつける | 制御文レベル |
| 6 | Stroustrup | 関数定義: Allman, 制御文: K&R | ケースバイケース |
| 7 | **none** | CMODE はブレーススタイルを仮定せず、すべての `{` と `}` の位置をそのまま尊重 | ユーザが配置した場所から逆算 |

```
set $c-brace-style 0   (K&R スタイル)
set $c-brace-style 1   (Allman スタイル)
set $c-brace-style 7   (none -- すべてのブレース配置を尊重)
```

### 4.4 `$c-cont-indent` (デフォルト: 2)

継続行 (括弧内の改行等) の追加インデントをスペース数で指定します。

```
set $c-cont-indent 4    (継続行に 4 スペース/タブ追加)
```

### 4.5 `$c-case-indent` (デフォルト: 0)

`switch` 内の `case` / `default` ラベルのインデントを `switch` からの相対値で指定します。

- `0` (デフォルト): `case` は `switch` と同じレベル
- `4`: `case` は `switch` から 4 スペースインデント

```
set $c-case-indent 4    (case を switch から 4 スペースインデント)
```

### 4.6 `$c-pp-indent` (デフォルト: 0)

プリプロセッサ指示文 (`#define` 等) のインデントをスペース数で指定します。

- `0` (デフォルト): プリプロセッサは列 0 に配置 (C の標準 convention)

```
set $c-pp-indent 4    (#define を 4 スペースインデント)
```

### 4.7 `$c-auto-close-brace` (デフォルト: TRUE)

閉じブレース `}` を入力したときに、対応する開きブレースのインデントレベルに自動的に戻るかどうかを指定します。

```
set $c-auto-close-brace FALSE   (自動デデント無効)
```

### 4.8 `$c-auto-detect` (デフォルト: FALSE)

ファイルを開く際、または `M-x c-detect-style` 実行時に、ソースコード内の既存インデントからブレーススタイル・インデント単位を自動検出するかどうかを指定します。

```
set $c-auto-detect TRUE    (ファイルオープン時に自動検出)
```

**検出アルゴリズム:**
1. ファイルの最初の 200 行をスキャン
2. タブのみ → `c_use_spaces = FALSE`
3. スペースのみ → `c_use_spaces = TRUE`, 最も頻出するリーダー長から `c_indent_width` を決定 (2/4/8)
4. タブ + スペース混在 → Google スタイル
5. ブレーススタイル検出: `if (...) {` 多い → K&R, `{` 独立行 → Allman, +2 カラム → GNU, 本文と同じ → Whitesmiths
6. 検出不能 → スタイル 7 (none), タブ, `c_indent_width = 8`

### 4.9 `$c-follow-source` (デフォルト: FALSE)

CMODE が前の行のインデントをそのまま尊重する「ソース追随モード」を有効にします。ユーザが手動で設定したインデント幅を無視し、既存コードに対して「そのまま続ける」ことを目的とします。

```
set $c-follow-source TRUE
```

### 4.10 `$c-gnu-indent-offset` (デフォルト: 2)

GNU スタイルで内部ブレースを半分インデントする際のカラム数を指定します。

```
set $c-gnu-indent-offset 4
```

### 4.11 `$c-brace-standalone-func` (デフォルト: FALSE)

Stroustrup スタイルで、関数定義の開きブレースを独立した行に配置するかどうかを指定します。

```
set $c-brace-standalone-func TRUE
```

### 4.12 `.kemacsrc` での永続設定

ホームディレクトリの `.kemacsrc` ファイルに以下を追加すると、kemacs 起動時に自動的に設定されます。

```lisp
; .kemacsrc -- CMODE の永続設定
; デフォルト: タブを使用
set $c-indent-width 4
set $c-use-spaces FALSE
set $c-brace-style 0
set $c-cont-indent 4
set $c-case-indent 0
set $c-pp-indent 0
set $c-auto-close-brace TRUE
set $c-auto-detect FALSE

; cindent を M-J にバインド
bind-to-key cindent M-J

; 領域再インデントを C-M-J にバインド
bind-to-key cindent-region C-M-J

; スタイル再検出をバインド (任意)
bind-to-key c-detect-style C-M-D

; CMODE を .c/.h/.cpp/.hpp/.cc/.cxx ファイルに自動適用
auto-mode-for-file CMODE "\\.c$"
auto-mode-for-file CMODE "\\.h$"
auto-mode-for-file CMODE "\\.cpp$"
auto-mode-for-file CMODE "\\.hpp$"
auto-mode-for-file CMODE "\\.cc$"
auto-mode-for-file CMODE "\\.cxx$"
```

---

## 5. サポートされる C 機能

### 5.1 対応キーワード

**ブロック開始キーワード** (次の行を 1 レベルインデント):

```
if  for  while  switch  do  else
try  catch  finally   (C23)
```

**デデントキーワード** (対応する親構造と同じレベル):

```
else  -- if と同じレベル
case  -- switch と同じレベル (または +c-case-indent)
default -- switch と同じレベル (または +c-case-indent)
```

### 5.2 コメント処理

- `/* */` ブロックコメント内の `{`, `(`, `[`, `#` は無視されます
- `//` 行コメント (C99) もサポートされています

### 5.3 文字列/文字リテラル

- `"..."` 文字列リテラル内の `{`, `(`, `[`, `#` は無視されます
- `'...'` 文字リテラルもサポートされています
- エスケープ Sequences (`\"`, `\\`, `\n` 等) を正しく処理します

### 5.4 プリプロセッサ

| 指令文 | インデント動作 |
|--------|---------------|
| `#if` / `#ifdef` / `#ifndef` | 次の行を +1 レベル (c-pp-indent 適用) |
| `#else` / `#elif` | `#if` と同じレベル |
| `#endif` | `#if` のレベルに戻る |
| `#define` (継続行 `\`) | 継続行として処理 |
| `#include` / `#pragma` / `#error` / `#warning` | 通常の文として扱われる |

### 5.5 C23 機能

| 機能 | インデント動作 |
|------|---------------|
| `[[attribute]]` | 属性リストは現在の行のインデントを維持 |
| `try` / `catch` / `finally` | ブロック開始キーワードとして扱う |
| `_BitInt(N)` | 通常の型として扱う |

---

## 6. ブレーススタイル詳細

CMODE は波括弧の配置をユーザの責務としています。`c_brace_style` は「`{` がどこにあるか」に基づいて「`}` や本文をどのようにインデントするか」を決定するヒントです。

### 6.1 スタイル 7 「none」

`c_brace_style = 7` を設定すると、CMODE はどんなブレーススタイルでも受け入れます。ユーザが `{` をどこに置こうが、`}` をどこに置こうが、その配置を尊重してインデントを計算します。

- 対応する `{` が行末なら → `}` は `{` と同じレベル (K&R, 1TBS など)
- 対応する `{` が独立行なら → `}` は `{` 行と同じレベル (Allman, Whitesmiths など)
- GNU スタイルの場合 → `}` は `{` と同じレベル

```
set $c-brace-style 7   (none -- すべてのブレース配置を尊重)
```

### 6.2 スタイル 6 「Stroustrup」

関数定義では Allman スタイル (`{` on own line), 制御文では K&R スタイル (`{` at end of line)。`$c-brace-standalone-func` と組み合わせて使用します。

### 6.3 スタイル 5 「1TBS」

K&R に近似し、`else` / `}` を前の行にくっつけるスタイルです。

---

## 7. 使用例

### 7.1 基本的なブロック構造 (K&R, タブ)

```c
if (x) {
	for (int i = 0; i < n; i++) {
		if (i > 5) {
			process(i);
		} else {
			skip(i);
		}
	}
} else {
	handle_error();
}
```

### 7.2 switch / case

```c
switch (value) {
case 1:
	do_one();
	break;
case 2:
	do_two();
	break;
default:
	do_default();
	break;
}
```

### 7.3 プリプロセッサ

```c
#if defined(DEBUG)
	#define LOG(x) printf(x)
#else
	#define LOG(x)
#endif
```

### 7.4 継続行

```c
result = function_call(arg1,
                       arg2,
                       arg3);
```

### 7.5 do-while

```c
do {
	process();
} while (condition);
```

### 7.6 C23 try/catch

```c
try {
	risky_operation();
} catch (err) {
	handle_error(err);
} finally {
	cleanup();
}
```

---

## 8. 実装の仕組み

### 8.1 インデント計算アルゴリズム

CMODE は以下のアプローチでインデントを計算します:

1. **前の行の解析**: 前の行の末尾文字 (`{`, `}`, `(`, `[`, `;`, 等) をチェック
2. **コメント/文字列スキップ**: `/* */`, `//`, `"..."`, `'...'` の内部を解析対象から除外
3. **キーワード認識**: C の制御文キーワード (`if`, `else`, `for`, `while`, `switch`, `case`, `do`, `try`, `catch`, `finally`) を認識
4. **ブレースレベル追跡**: `c_scan_for_brace_level()` が後方スキャンで対応する `{` のインデントを検索
5. **プリプロセッサ対応**: `#if`/`#endif`/`#else`/`#elif` のネストを追跡

### 8.2 cindent-region のアルゴリズム

`cindent-region` はインデントスタックを使用した前方向スキャンを行います:

```
1. 領域開始前の行からベースインデントを取得
2. スタックを初期化 (indent_stack[0] = base_indent, sp = 0)
3. 各行をスキャン:
   a. `}` があればスタックをポップ
   b. `{` があればスタックにプッシュ (col + c-indent-width)
   c. `case`/`default` は switch レベル + c-case-indent
   d. `else` は対応する if と同じレベル
   e. `#if`/`#ifdef`/`#ifndef` は pp_sp をインクリメント
   f. `#endif` は pp_sp をデクリメント
4. スタックに基づいて各行のインデントを計算
```

### 8.3 インデント単位の選択 (スペース vs タブ)

`c_insert_indent()` 関数は `c_use_spaces` の値に基づいてインデントを生成します:

- `c_use_spaces = TRUE`: 指定された列数だけスペースを挿入
- `c_use_spaces = FALSE`: タブ (tabsize で割り切れる部分) + スペース (余り部分) を挿入

```
/* c_insert_indent() in cmode.C_ */
if (c_use_spaces) {
    /* Insert 'cols' spaces */
    for (i = 0; i < cols; i++)
        linsert(1, ' ');
} else {
    /* Insert tabs for full tab stops, spaces for partial */
    int tabs = cols / TS;
    int spaces = cols % TS;
    for (i = 0; i < tabs; i++)
        linsert(1, '\t');
    for (i = 0; i < spaces; i++)
        linsert(1, ' ');
}
```

---

## 9. トラブルシューティング

| 症状 | 原因 | 解決方法 |
|------|------|---------|
| インデントが正しくない | `$c-indent-width` が 0 | `set $c-indent-width 4` |
| スペースが挿入される | `$c-use-spaces` が TRUE | `set $c-use-spaces FALSE` (タブを使う) |
| タブが挿入される | `$c-use-spaces` が FALSE | `set $c-use-spaces TRUE` (スペースを使う) |
| `case` が switch に揃わない | `$c-case-indent` が 0 以外 | `set $c-case-indent 0` |
| `}` が正しくない位置に | `$c-auto-close-brace` が FALSE | `set $c-auto-close-brace TRUE` |
| CMODE が機能しない | MDCMOD ビットが off | `M-x c-mode` でON |
| スタイルが検出されない | `$c-auto-detect` が FALSE | `set $c-auto-detect TRUE` または `M-x c-detect-style` |
| 自動検出でタブが使われない | ソースがスペースのみ | 自動検出はソースのリーダーを分析します |
