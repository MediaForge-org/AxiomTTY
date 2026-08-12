# M1 Terminal Core R3 — Manual Test Checklist

This milestone is still pre-alpha. The goal is to find terminal-emulation gaps, not to claim full xterm compatibility.

## 1. Basic shell and PTY

```bash
pwd
whoami
uname -a
ls -la
echo "hello"
git --version
```

## 2. Colors

```bash
printf '\e[31mRED\e[0m  \e[32mGREEN\e[0m  \e[34mBLUE\e[0m\n'
printf '\e[38;2;255;120;40mTRUECOLOR\e[0m\n'
ls --color=always
```

## 3. Resize

```bash
stty size
```

Resize the window and run `stty size` again. Rows and columns should change.

## 4. Ctrl+C behavior

Without a selection:

```bash
sleep 30
```

Press `Ctrl+C`. The process should stop.

With a selection: drag across terminal text with the left mouse button, press `Ctrl+C`, then paste the clipboard somewhere. The selected text should be copied and no `^C` should be sent to the shell.

`Ctrl+Shift+C` remains an optional copy alias.

## 5. Scrollback

```bash
seq 1 300
```

Use the mouse wheel or `Shift+PageUp` / `Shift+PageDown` to inspect older output. Typing a normal command should return the viewport to the bottom.

Text selection and `Ctrl+C` should also work while viewing scrollback.

## 6. Job control

```bash
sleep 30
```

Press `Ctrl+Z`, then test:

```bash
jobs
fg
```

## 7. sudo / password input

```bash
sudo -v
```

The password prompt should work and typed password characters must not be echoed.

## 8. Alternate-screen applications

Try any that are installed:

```bash
less README.md
nano
vim
htop
btop
```

Look for cursor errors, stale characters, bad colors, broken resizing, input problems, or failure to restore the normal screen after quitting.

## Known limitations

- no terminal mouse-reporting protocols yet
- no Unicode wide-character / combining-character cell-width model yet
- selection does not yet understand soft-wrapped logical lines
- no polished scrollbar UI yet
- full xterm/VT compatibility is not expected yet
