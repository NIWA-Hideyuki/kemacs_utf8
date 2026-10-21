# kemacs Python Mode 使用方法

kemacs に組み込まれた **Python Mode (PYMODE)** は、Python のインデンテーションを自動的に計算・挿入・調整する機能です。コロン (`:`)、バックスラッシュ (`\`)、キーワード (`else:`, `elif:`, `except:`, `finally:`) を解析して適切なインデントを決定します。

---

## 1. 機能概要

| 機能 | コマンド名 | キー | 説明 |
|------|-----------|------|------|
| 自動インデント (改行時) | `pyinsert` | `Enter` | 行末のコロンやバックスラッシュに基づいて新しい行のインデントを自動計算 |
| 行の再インデント | `py-indent` | `M-J` | カレント行のインデントを前行の文脈に基づいて修正 |
|| 領域の再インデント | `py-format-region` | `M-x` | 選択リージョン内の各行を Python 構文に基づいて再インデント |
| バックスペース (デデント) | `py-backspace` | `Backspace` | インデント領域でバックスペースを押すと 1 インデントレベル分削除 |

---

## 2. Python Mode の有効化

Python Mode は **バッファローカル** なモードです。以下の方法で有効にできます。

### 2.1 ファイル拡張子による自動判定

`.py` 拡張子のファイルを開くと、`MAGIC` が有効な場合に自動的に Python Mode がONになります。

### 2.2 手動で有効化

```
M-x py-mode    (py-mode と入力)
```

`M-x` は「名前を指定してコマンドを実行する」コマンド (namedcmd) です。`py-mode` と入力すると、カレントバッファの Python Mode (PYMODE) が ON/OFF 切り替えられます。

---

---

## 3. キー操作

### 3.1 `Enter` (pyinsert) — 自動インデント

Python Mode がONのバッファで `Enter` を押すと、前行の内容に基づいて新しい行のインデントが自動計算されます。

**ルール:**
- 前行が `:` で終わる場合 → 1 インデントレベル (py-indent-width) 増やす
- 前行が `\` (バックスラッシュ) で終わる場合 → 同じインデントを継承
- それ以外 → 前行と同じインデント

```
if True:          ──Enter──▶
    x = 1          │
                   ▼    ← 4 スペース自動挿入
```

### 3.2 `M-J` (py-indent) — 行の再インデント

カーソル行のインデントを、前の非空行の文脈に基づいて再計算します。

**ルール:**
- 前行が `:` で終わる → 1 レベル追加
- キーワード (`else:` / `elif:` / `except:` / `finally:`) → 1 レベル削減
- バックスラッシングによる継続行 → 前行と同じインデント

```
if True:
    x = 1
    else:         ──M-J──▶
                │    ▼
                │  else:   ← デデント (4 スペース削除)
```

### 3.3 `M-x py-format-region` (py-format-region) — 領域の再インデント

リージョン (範囲選択) 内の各行を、Python のブロック構造に基づいて再インデントします。インデントスタックを使用して、入れ子構造を正しく解析します。

**使用方法:**
1. `M-space` でマークを設定 ("[マーク 設定]")
2. カーソルを移動して再インデントしたい範囲を選択
3. `M-x` を押して `py-format-region` と入力

### 3.4 `Backspace` (py-backspace) — デデント

Python Mode がONのバッファで、行頭の空白領域内で `Backspace` を押すと、1 インデントレベル分の空白が削除されます。

- スペースモード (`py-use-spaces = TRUE`): `py-indent-width` スペースを 1 回で削除
- タブモード (`py-use-spaces = FALSE`): 1 タブを削除

空白領域以外で `Backspace` を押した場合は、通常のバックスペース (1 文字削除) が動作します。

---

## 4. 設定変数

Python Mode の動作は以下の 2 つのグローバル変数で制御します。

### 4.1 `$py-indent-width` (デフォルト: 4)

インデントの幅をスペース数で指定します。PEP 8 の推奨値は 4 です。

```
set $py-indent-width 2    (2 スペースインデント)
set $py-indent-width 8    (8 スペースインデント)
```

### 4.2 `$py-use-spaces` (デフォルト: TRUE)

インデントにスペースを使用するかどうかを BOOL 値で指定します。

- `TRUE` (デフォルト): スペースを使用
- `FALSE`: タブを使用 (タブ幅は `$tabsize` または環境変数 `$tabsize` に依存)

```
set $py-use-spaces FALSE   (タブモード)
set $py-use-spaces TRUE    (スペースモード)
```

### 4.3 `.kemacsrc` での永続設定

ホームディレクトリの `.kemacsrc` ファイルに以下を追加すると、kemacs 起動時に自動的に設定されます。

```lisp
; .kemacsrc
set $py-indent-width 4
set $py-use-spaces TRUE
```

## 5. 使用例

### 5.1 基本的なインデント

```
if True:
    for i in range(10):
        if i > 5:
            print(i)
        else:
            print("skip")
```

### 5.2 .kemacsrc 設定

```lisp
; .kemacsrc — Python Mode の永続設定
set $py-indent-width 4
set $py-use-spaces TRUE
```

