# Roadmap

Milestone labels are used during pre-alpha development. A public semantic
version such as 0.1.0 is intentionally deferred until the application is
actually useful as a daily terminal.

## M0 — Bootstrap / PTY input — DONE
- C++23 / CMake / Qt 6 Quick
- native Linux PTY
- launch `$SHELL`
- direct keyboard input to PTY
- first restrained UI design system

## M1 — Terminal core — IN PROGRESS
Completed through M1 R4:
- VT parser foundation
- screen/cell buffer
- SGR 16/256/true-color
- cursor movement and visibility
- alternate screen
- bracketed paste
- application cursor mode
- resize tied to rendered cell geometry
- basic terminal responses and OSC title
- custom C++ terminal surface
- visible scrollback viewport (mouse wheel and Shift+PageUp/PageDown)
- cell-based mouse selection and clipboard copy
- context-sensitive `Ctrl+C`: copy selection or send Unix `^C`

Remaining M1 refinements:
- Unicode width/combining handling
- scrollback search and polished scrollbar UI
- soft-wrap-aware selection
- mouse reporting
- more xterm/DEC modes
- performance renderer refinement

Compatibility probes: `sudo`, `ssh`, `nano`, `vim`, `htop`, `btop`, `less`.
Passing every probe perfectly is the M1 exit criterion, not a promise for an individual R1/R2/R3/R4 refinement.

## M2 — Sessions, tabs and splits
- many simultaneous PTYs
- horizontal/vertical split tree
- session lifecycle
- tab renaming/pinning/reordering
- notifications for completed long-running commands

## M3 — Product-quality interaction
- searchable history
- completion UI
- link/path detection
- find in scrollback
- command palette
- profiles and themes

## M4 — Own Linux-style shell
- lexer and parser
- built-ins (`cd`, `pwd`, `export`, `alias`, `history`, `jobs`, ...)
- arbitrary external executables through `$PATH`
- pipes and redirects
- `&&`, `||`, `;`, background jobs
- variables, quoting, globbing
- job control and foreground process groups
- completion API

The system shell remains available even after the own shell becomes usable.

## M5 — Linux integration
- `/proc` process model
- files/mounts
- systemd service integration
- journal view
- network information
- optional file browser

## M6 — Packages
- native DNF integration
- Flatpak integration
- optional unified `pkg` command
- GUI package view
- privilege/authentication through normal Linux mechanisms

## M7 — Workspaces and extensibility
- saved layouts, working directories and connections
- SSH profiles
- plugin/extension API
- completion providers
