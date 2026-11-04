# kemacs on WSL2: `^V` (Ctrl+V) を使えるようにする

kemacs の `^V` は「次ページへスクロール」(`forwpage`) です (`M-V` / `ESC V` が前ページ)。
WSL2 環境では Windows 側のターミナルが `Ctrl+V` を「貼り付け」に割り当てているため、
キーが kemacs まで届かず `^V` が使えません。本ドキュメントはその回避方法 (ターミナル設定) をまとめます。

---

## 1. Windows Terminal (推奨) の設定

### GUI で設定する場合

1. Windows Terminal を起動する
2. 設定を開く (`Ctrl + ,`)
3. 「操作 (Actions)」タブを選択
4. 「貼り付け (Paste)」に割り当てられている `Ctrl+V` を `Ctrl+Shift+V` に変更する。
5. 保存して Windows Terminal を再起動する

### settings.json を直接編集する場合

設定ファイルの場所:

```
%LOCALAPPDATA%\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json
```

`"actions"` (古いバージョンでは `"keybindings"`) 配列から、`Ctrl+V` を貼り付けに割り当てている
エントリを削除します。例:

```json
"actions": [
    { "command": "paste", "keys": "ctrl+shift+v" }
]
```

`"keys": "ctrl+v"` を含む貼り付けエントリが残っていると、`Ctrl+V` は kemacs に届きません。

---

## 2. 従来のコンソール (conhost) の場合

1. コンソールウィンドウのタイトルバーを右クリック → 「プロパティ」
2. 「オプション」タブを開く
3. 以下のいずれかを行う
   - 「Ctrl キーのショートカットを有効にする」のチェックを外す
   - または「Ctrl+Shift+C/V をコピー/貼り付けに使用する」をオンにする
4. 「クイック編集モード」がオンの場合はオフにする (マウス選択による意図しないコピーを防ぐ)
5. 「OK」で保存

---

## 3. Linux / WSL 側の tty 設定 (補助)

kemacs は raw モードで動作するため通常不要ですが、シェルから起動する前や tmux/screen 経由で
使う場合は以下を確認してください。

```sh
# XON/XOFF フロー制御を無効化 (Ctrl+S / Ctrl+Q のため)
stty -ixon

# literal-next (^V) が lnext として登録されていることを確認
stty lnext '^V'
```

永続化する場合は `~/.bashrc` / `~/.zshrc` に追記します。

```sh
stty -ixon 2>/dev/null
```

---

## 4. tmux / GNU screen を使う場合

tmux が `Ctrl+V` を横取りしている場合は `~/.tmux.conf` に以下を追加します。

```tmux
# Ctrl+V を tmux で消費せず下層アプリに渡す
bind-key -n C-v send-keys C-v
```

GNU screen の場合は `.screenrc` で `bindkey` による横取りがないことを確認してください。

---

## 5. 動作確認

### 5.1 端末に ^V が届くか確認

```sh
stty raw -echo
od -c        # ここで Ctrl+V を押し、続けて Ctrl+C で中断
stty sane
```

`026` (8 進) / `^V` が表示されれば、端末設定は正常です。

### 5.2 kemacs での確認

```sh
TERM=xterm-256color kemacs somefile.txt
```

ファイルを開いた状態で `^V` を押して「次ページへスクロール」が動作することを確認します。
代わりに貼り付けが起きる場合は、Windows Terminal / conhost の設定 (セクション 1 / 2) を
再確認してください。

---

## 6. その他の注意点

- 日本語 IME が有効な状態では `Ctrl+V` が IME に消費される場合があります。
  英数 (直接入力) モードにしてから操作してください。
- kemacs の `^V` は `forwpage`、`M-V` (`ESC V`) は `backpage` です。
  ページ移動には `^V` / `M-V` を使用します。

---

## 関連

- `termio.C_`: `ttopen()` で raw モード設定 (`cfmakeraw`)
- `tcap.C_`: 端末機能の初期化 (`TERM_MODERN` で bracketed paste 等)
- `docs/kemacs使用手引書.md`: 全般的な使用方法
