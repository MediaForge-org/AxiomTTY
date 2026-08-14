# AxiomTTY — M3.1.3 Settings Interaction Reliability (Pre-Alpha)

AxiomTTY is a Linux-first terminal emulator being developed in C++23 with Qt 6/QML.
Linux/POSIX behavior is the reference; Fedora is the first development platform.

The project is intentionally still **pre-alpha**. Milestone labels are used instead
of public semantic versions until AxiomTTY is useful as a daily terminal.

## Current capabilities

- native Linux PTYs and the real user `$SHELL`
- custom C++ VT/xterm screen model and renderer
- 16/256/True Color, Unicode/wide cells, common DEC/xterm modes and TUI support
- tested interactively with tools such as `less`, `nano`, `htop` and `btop`
- fixed internal scrollback retention (5000 rows for now) with cross-history mouse selection and edge auto-scroll
- context-sensitive `Ctrl+C`: copy selection or send Unix `^C`
- desktop-style `Ctrl+V` paste
- multiple independent terminal tabs
- arbitrarily nested horizontal/vertical split panes with one PTY per leaf
- draggable split handles and independent PTY resize
- working-directory inheritance for new tabs, duplicates and split panes
- custom tab names, automatic-title reset and tab lifecycle context menu
- pane-local **full-scrollback search** with `Ctrl+F`
- highlighted search results, current/total counter and next/previous navigation
- case-sensitive search option and soft-wrap/wide-character-aware matching
- geometric four-direction pane focus navigation
- active-pane duplication with shell/CWD inheritance and a fresh PTY
- close protection when panes/tabs/application still contain child processes
- natural lifecycle: closing the last pane closes its tab; closing the last tab closes AxiomTTY
- persistent settings in `~/.config/axiomtty/settings.ini`
- live terminal font family/size settings with viewport-stable font resizing
- configurable default shell, start directory, CWD inheritance and close protection

## Main shortcuts

```text
Ctrl+Shift+T        new tab
Ctrl+Shift+W        close active tab
Ctrl+Tab            next tab
Ctrl+Shift+Tab      previous tab
Alt+1 ... Alt+9     activate tab 1 ... 9

Ctrl+Shift+D        split active pane to the right
Ctrl+Shift+E        split active pane downward
Ctrl+Shift+X        close active pane (asks if a child process is running)
Ctrl+Shift+Left     focus pane to the left
Ctrl+Shift+Right    focus pane to the right
Ctrl+Shift+Up       focus pane above
Ctrl+Shift+Down     focus pane below
Alt+Shift+D         duplicate active pane to the right
Alt+Shift+E         duplicate active pane downward

Ctrl+F              search active pane scrollback
Ctrl+,              open settings
Enter / F3          next search result
Shift+Enter / Shift+F3  previous search result
Escape              close search
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

## M3.1.3 settings interaction reliability

- Settings writes are debounced instead of calling `QSettings::sync()` inside UI input events.
- `Reset defaults` runs on the next event-loop turn and exposes a short `Resetting…` state.
- Reset completion is signalled by the C++ settings object; the dialog resynchronizes only after the reset is complete.
- This avoids re-entering QML controls while their click/press event is still being processed.


## M3.1 manual test

See `docs/TESTING_M3_1.md`. M2.5 lifecycle and search regressions remain relevant. Search from M2.4 remains available; a useful history generator is:

```bash
for i in $(seq 1 1500); do
    printf 'line %04d  alpha beta %s\n' "$i" "$([ $((i % 125)) -eq 0 ] && echo ERROR || echo ok)"
done
```

Then press `Ctrl+F` and search for `ERROR`.

## Design direction

AxiomTTY should remain recognizably a Linux terminal rather than becoming a card-heavy
IDE or AI-style dashboard. The terminal stays visually dominant; tabs, splits, search
and later workspace tools remain compact infrastructure around it.
