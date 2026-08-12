# AxiomTTY — M1 TUI Compatibility R8 (Pre-Alpha)

AxiomTTY is a Linux-first terminal emulator being developed in C++23 with Qt 6/QML.
Linux/POSIX behavior is the reference; Fedora is the first development platform.

The project is intentionally still **pre-alpha**. Milestone/revision labels are used
instead of public semantic versions until AxiomTTY is useful as a daily terminal.

## Current M1 R8 capabilities

- native Linux PTY process execution via `forkpty()`
- starts the user's real `$SHELL`
- arbitrary installed Linux programs run through the normal environment and `$PATH`
- direct keyboard input, control characters and common function/navigation keys
- custom C++ terminal screen/cell model and renderer
- incremental UTF-8 decoding
- single-width, double-width and combining-character cell handling
- ANSI/VT cursor movement, erase, insert/delete and scroll operations
- 16-color, 256-color and True Color SGR
- bold, italic, underline and inverse attributes
- primary and alternate screen buffers
- DEC Special Graphics character set for curses-style line drawing
- DECSCUSR block/underline/bar cursor shapes with blinking/steady modes
- application cursor-key mode
- origin mode and ANSI insert mode
- bracketed paste
- OSC 0 / OSC 2 terminal titles
- basic device/status reports
- xterm mouse tracking foundations (`1000`, `1002`, `1003`, `1006`)
- focus reporting (`1004`)
- local text selection across the full scrollback history; hold **Shift** to select when a TUI owns the mouse
- edge auto-scroll while dragging a selection above/below the visible viewport
- selections survive manual scrollback navigation and can be copied while partially/off-screen
- configurable horizontal tab stops (HTS/TBC/CHT/CBT) with standard eight-column defaults
- CSI REP repeat-character support
- SGR faint and strikethrough rendering
- xterm alternate-scroll mode (`1007`) for wheel-to-arrow behavior on alternate screens
- synchronized-output mode (`2026`) with repaint batching to reduce TUI flicker
- xterm-style OSC 10/11 default foreground/background color query responses
- richer modified Insert/Delete/Page/F-key sequences
- cursor save/restore now preserves rendition and key cursor-state attributes
- `Ctrl+C` copies an active selection, otherwise sends normal Unix `^C`
- `Ctrl+V` pastes; `Ctrl+Shift+C/V` remain aliases
- 5000-line scrollback with mouse wheel and `Shift+PageUp/PageDown`
- soft-wrap-aware copied text
- improved vertical resize behavior that preserves recent visible rows in scrollback
- PTY, interactive-shell and VT-core smoke tests
- Fedora 44 GitHub Actions build/test workflow
- frameless AxiomTTY-owned dark window chrome on Qt 6.8+
- compositor-driven move/resize via Qt system move/resize APIs
- compact unified product/session header and quiet status bar
- transient terminal-size feedback during resize

## Build on Fedora

```bash
./scripts/install-fedora.sh   # only needed once
./scripts/build.sh
./build/axiomtty
```

`build.sh` performs a clean Debug build and then runs `ctest`.

## Useful compatibility tests

```bash
printf '\e[31mRED\e[0m  \e[32mGREEN\e[0m  \e[38;2;120;180;255mTRUECOLOR\e[0m\n'
ls --color=always
clear
printf '\e]2;AxiomTTY title test\a'
printf 'ASCII  |  中文  |  e\u0301  |  €\n'
```

Then use real terminal applications as probes:

```bash
sudo -v
less README.md
nano
vim
htop
btop
```

M1 is not finished yet. Complex emoji grapheme clusters, complete xterm/DEC
compatibility, polished resize reflow, scrollback search and renderer performance
work still remain.

See `docs/TESTING_M1_R8.md` for the current manual test pass.

## Design direction

AxiomTTY is intended to remain recognizably a Linux terminal rather than becoming a
card-heavy dashboard. The GUI is deliberately restrained, desktop-oriented and
separate from the terminal/process core.
