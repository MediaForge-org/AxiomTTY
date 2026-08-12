# M2.1 manual test — Tabs & Sessions

## 1. Create independent tabs

Start AxiomTTY. Press `Ctrl+Shift+T` three times. Four tabs should exist and each
should be interactive.

In different tabs run:

```bash
printf 'TAB A\n'
sleep 60
```

```bash
printf 'TAB B\n'
pwd
```

The `sleep` in one tab must not block input in another tab.

## 2. Working-directory inheritance

In tab 1:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
pwd
```

Wait briefly for the tab label to become `AxiomTTY`, then press `Ctrl+Shift+T` and run:

```bash
pwd
```

The new tab should start in `/mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY`.

Repeat from `/tmp` and from the home directory.

## 3. Independent terminal state

In tab 1:

```bash
seq 1 500
```

Scroll upward. Switch to tab 2 and produce different output. Returning to tab 1 should
show its own terminal history; the screen contents must not leak between tabs.

## 4. TUI independence

Run `btop`, `htop`, `nano` or `vim` in one tab. Switch to another tab and run ordinary
shell commands. Switch back. The TUI should still be alive and redraw correctly.

## 5. Navigation

Test:

- clicking tabs
- `Ctrl+Tab` / `Ctrl+Shift+Tab`
- `Ctrl+PageDown` / `Ctrl+PageUp`
- `Alt+1` ... `Alt+9`
- enough tabs that the tab strip becomes horizontally scrollable

## 6. Closing

Close tabs using the `×`, middle-click, and `Ctrl+Shift+W`.

A tab running `sleep 600` should disappear without affecting other tabs. Closing the
last tab should immediately create a fresh shell tab.

## 7. Preserve normal terminal Ctrl+W

At a Bash prompt type:

```text
one two three
```

without pressing Enter. Press `Ctrl+W`. Bash/readline should delete the previous word.
AxiomTTY must not interpret plain `Ctrl+W` as "close tab".

## 8. Existing regression checks

Verify that selection, scrollback, `Ctrl+C`, `Ctrl+V`, resize, `less`, `nano`, `htop`
and `btop` still behave as in M1 R8.
