# AxiomTTY — M3.2.7 Pane Close & Focus Polish (Pre-Alpha)

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
- balanced same-axis pane sizing with an 8-pane per-tab ceiling
- explicit per-pane close control with deterministic nearest-neighbor focus after close
- draggable split handles and independent PTY resize
- deterministic profile start directories for fresh tabs; duplicates and split panes preserve the current CWD
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
- configurable default shell, profile start directories and close protection
- persistent terminal profiles with per-profile shell, start directory and color scheme
- built-in `Default`, `Development` and `Server` profiles plus custom profiles
- four visually distinct terminal color schemes: Axiom Dark, Midnight, Graphite and Forest
- reliable live recoloring of existing panes when their profile theme is applied
- batched profile editing to keep the Settings dialog responsive
- fresh tabs always honor the selected profile start directory; duplication/splits preserve the current CWD
- right-click the `+` button to open a tab with a specific profile
- Settings edits stay in memory while the UI is open; disk persistence is flushed on shutdown to avoid interaction stalls
- `+` / `Ctrl+Shift+T` always starts at the selected profile's configured start directory

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

## M3.2.3 deterministic new-tab semantics

- Profiles are persistent C++ settings objects rather than QML-only presets.
- Each profile owns a shell, start directory and terminal color scheme.
- New tabs use the configured default profile; right-clicking `+` lets you choose another profile explicitly.
- Split panes inherit the profile of their tab.
- Existing panes update live when that profile's color scheme is applied.
- Profile shell/start-directory/theme edits are staged in the Settings dialog and applied as one batch.
- Fresh tabs always use the selected profile start directory. Duplicate Tab and split/duplicate pane actions preserve the active CWD explicitly.
- Terminal schemes remap the default and base ANSI palette while preserving 256-color/True Color output semantics.
- Built-in profiles are protected from deletion; custom profiles can be added and removed.

## M3.2.1 manual test

See `docs/TESTING_M3_2_3.md`. A quick profile/theme probe is:

```bash
printf '\e[31mRED\e[0m \e[32mGREEN\e[0m \e[34mBLUE\e[0m \e[38;2;255;120;40mTRUECOLOR\e[0m\n'
```

Open Settings with `Ctrl+,`, switch a profile between Axiom Dark / Midnight / Graphite / Forest, and verify already-open panes using that profile recolor without restarting their shell.

## Design direction

AxiomTTY should remain recognizably a Linux terminal rather than becoming a card-heavy
IDE or AI-style dashboard. The terminal stays visually dominant; tabs, splits, search
and later workspace tools remain compact infrastructure around it.

## M3.2.4 non-blocking Settings persistence

Settings/profile changes no longer perform QSettings writes or syncs while the user is interacting with the Settings UI. Mutations update the in-memory model immediately, so a changed profile start directory is available to the very next tab, while the INI write is batched until shutdown. Explicit `flush()` remains available for tests and controlled shutdown.
