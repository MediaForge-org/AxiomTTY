# AxiomTTY — M2.3 Session Polish (Pre-Alpha)

AxiomTTY is a Linux-first terminal emulator being developed in C++23 with Qt 6/QML.
Linux/POSIX behavior is the reference; Fedora is the first development platform.

The project is intentionally still **pre-alpha**. Milestone labels are used instead
of public semantic versions until AxiomTTY is useful as a daily terminal.

## Current capabilities

### Terminal core

- native Linux PTY execution via `forkpty()`
- real user `$SHELL` and arbitrary installed Linux programs through `$PATH`
- custom C++ VT/xterm terminal screen model and renderer
- ANSI/VT cursor, erase, insert/delete, scrolling and alternate screen
- 16/256/True Color, common SGR attributes and DEC line graphics
- Unicode single/double-width cells and combining marks
- bracketed paste, application cursor mode and common xterm/DEC modes
- TUI compatibility foundations used successfully with `less`, `nano`, `htop` and `btop`
- 5000-line scrollback, cross-history mouse selection and edge auto-scroll
- `Ctrl+C` copies a selection or sends Unix `^C` when nothing is selected
- `Ctrl+V` desktop-style paste

### M2.1–M2.3 — tabs, sessions, split panes and polish

- multiple simultaneous tabs backed by independent PTYs
- each tab owns its own `TerminalSession`, screen buffer and running process tree
- new tabs inherit the active shell's current Linux working directory via `/proc/<pid>/cwd`
- tab labels follow the current working directory and OSC terminal titles
- close buttons and middle-click tab close
- keyboard tab creation, closing and navigation
- active shell/directory and tab count in the status bar
- closing the final tab creates a fresh shell instead of leaving a dead window
- binary split-tree layout supporting nested horizontal and vertical splits
- every split leaf is an independent PTY/session, not a duplicated view
- new split panes inherit the focused pane's current working directory
- draggable split handles resize the underlying PTYs independently
- active-pane focus marker, pane count per tab and pane navigation shortcuts
- closing a nested pane collapses its branch without disturbing sibling layouts
- custom tab names via double-click or right-click → Rename Tab
- right-click tab menu with duplicate, rename, automatic-title reset and close
- duplicated tabs inherit the selected tab's active shell and Linux working directory
- active-pane status now shows `PANE x/y` for split layouts
- inactive panes use subtle focus chrome instead of `CLICK TO TYPE` debug badges

## Tab and pane shortcuts

```text
Ctrl+Shift+T        new tab
Ctrl+Shift+W        close active tab
Ctrl+Tab            next tab
Ctrl+Shift+Tab      previous tab
Ctrl+PageDown       next tab
Ctrl+PageUp         previous tab
Alt+1 ... Alt+9     activate tab 1 ... 9

Ctrl+Shift+D         split active pane to the right
Ctrl+Shift+E         split active pane downward
Ctrl+Shift+X         close active pane
Ctrl+Shift+Right     next pane
Ctrl+Shift+Left      previous pane
```

`Ctrl+W` is deliberately **not** stolen by the GUI; it continues to reach Bash/readline
and terminal applications normally.

## Build on Fedora

```bash
./scripts/install-fedora.sh   # only needed once
./scripts/build.sh
./build/axiomtty
```

`build.sh` performs a clean Debug build and runs the automated tests.

## M2.3 manual test

See `docs/TESTING_M2_3.md`. M2.2 split regression checks remain relevant in `docs/TESTING_M2_2.md`. One important check is working-directory inheritance for a duplicated tab:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
# right-click the tab → Duplicate Tab
pwd
```

The duplicated tab should open in the same directory while remaining an independent shell.

## Design direction

AxiomTTY should remain recognizably a Linux terminal rather than becoming a card-heavy
IDE or AI-style dashboard. The terminal stays visually dominant; tabs, splits and later
workspace tools are compact infrastructure around it.
