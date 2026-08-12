# AxiomTTY M1 R8 — TUI compatibility manual test

R8 moves the M1 terminal core toward everyday compatibility with fullscreen Linux
terminal applications. It keeps the R7 selection/scrollback behavior and adds
several xterm/DEC details that editors and curses applications commonly expect.

## 1. Build and launch

```bash
./scripts/build.sh
./build/axiomtty
```

The automated smoke tests must pass before the window opens.

## 2. Basic terminfo sanity

```bash
printf 'TERM=%s\nCOLORTERM=%s\nTERM_PROGRAM=%s\n' "$TERM" "$COLORTERM" "$TERM_PROGRAM"
tput colors
tput cols
tput lines
```

Expected direction:
- `TERM=xterm-256color`
- `COLORTERM=truecolor`
- `TERM_PROGRAM=AxiomTTY`
- `tput colors` reports at least 256
- rows/columns track the actual AxiomTTY viewport

## 3. Tab stops and REP

Default tab stops should be eight columns apart:

```bash
printf 'A\tB\tC\n'
```

Repeat-character support:

```bash
printf 'X\e[12b\n'
```

The second command should render thirteen `X` characters total.

## 4. Additional SGR attributes

```bash
printf '\e[2mFAINT\e[22m normal  \e[9mSTRIKE\e[29m normal\n'
```

`FAINT` should be visibly dimmer and `STRIKE` should be struck through.

## 5. Synchronized-output batch

```bash
printf '\e[?2026h\e[2J\e[Hsync frame\e[?2026l'
```

The final frame should appear normally. R8 suppresses intermediate repaints while
DEC private mode 2026 remains active, reducing visible tearing/flicker in TUIs that
use synchronized output.

## 6. `less`

From the repository directory:

```bash
less README.md
```

Check:
- full-screen entry is clean
- arrows/PageUp/PageDown scroll correctly
- resize while `less` is open
- `q` returns to the previous primary-screen contents without debris

## 7. `nano`

If installed:

```bash
nano /tmp/axiomtty-r8.txt
```

Check:
- bottom shortcut bar and borders line up
- cursor position matches edited text
- Ctrl shortcuts work
- resizing redraws the whole screen correctly
- exit returns cleanly to the shell

## 8. `vim`

If installed:

```bash
vim -Nu NONE /tmp/axiomtty-r8.txt
```

Check:
- `i`, text entry and Escape
- arrow keys in normal/insert mode
- status line placement
- `:set mouse=a` and mouse clicks/scrolling
- Shift+drag still forces local AxiomTTY text selection when Vim owns the mouse
- resize while Vim is running
- `:q!` restores the primary screen

## 9. `htop` / `btop`

Run whichever is installed:

```bash
htop
btop
```

Check:
- box drawing is aligned
- colors and highlighting are sensible
- mouse interaction works when enabled by the application
- mouse wheel behaves as the TUI expects
- no stale fragments remain after resize
- exit restores the shell screen

## 10. Alternate-screen wheel mode

Some TUIs enable xterm alternate-scroll mode (`?1007`). R8 maps the wheel to cursor
up/down while the alternate screen owns the viewport and no explicit mouse protocol
is active. This should make pager/editor scrolling feel closer to xterm-compatible
terminals.

## 11. R7 regressions

Re-test the established interaction rules:

```bash
seq 1 3000
sleep 60
```

- multi-page selection + auto-scroll still works
- Ctrl+C copies when a selection exists
- Ctrl+C interrupts `sleep 60` when there is no selection
- Ctrl+V pastes
- primary scrollback remains usable with the mouse wheel and Shift+PageUp/PageDown

## Report useful failures

For TUI failures, note the program and the symptom rather than only “doesn't work”,
for example:

```text
vim: status line shifted one column after resize
btop: mouse click lands one row too high
less: primary screen not restored after q
nano: box drawing good, cursor wrong after inserting wide Unicode
```

Those reports map directly to the next VT/core fixes.
