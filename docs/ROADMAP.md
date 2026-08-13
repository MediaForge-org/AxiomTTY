# Roadmap

Milestone labels are used during pre-alpha development. A public semantic version
such as `0.1.0` is intentionally deferred until AxiomTTY is actually useful as a
daily terminal.

## M0 — Bootstrap / PTY input — DONE
- C++23 / CMake / Qt 6 Quick
- native Linux PTY
- launch `$SHELL`
- direct keyboard input to PTY
- first restrained UI design system

## M1 — Terminal core — IN PROGRESS

Completed through M1 R8:
- VT parser foundation
- screen/cell buffer
- incremental UTF-8 decoding
- single-width, double-width and combining-character cell handling
- SGR 16/256/True Color
- cursor movement, visibility and DECSCUSR cursor shapes
- alternate screen
- DEC Special Graphics line drawing
- bracketed paste
- application cursor mode
- origin mode and ANSI insert mode
- basic device/status responses and OSC title
- xterm mouse modes 1000/1002/1003 plus SGR mouse mode 1006
- xterm focus reporting mode 1004
- custom C++ terminal surface
- PTY resize tied to rendered cell geometry
- improved vertical resize preservation
- 5000-line scrollback viewport
- cell-based mouse selection and clipboard copy across full scrollback history
- edge auto-scroll while extending selections beyond the viewport
- selection persistence while manually navigating scrollback
- xterm/DEC tab-stop controls and CSI REP
- SGR faint/strikethrough rendering
- alternate-scroll mode 1007
- synchronized-output mode 2026 with repaint batching
- OSC 10/11 default color queries
- richer modified navigation/function key reporting
- soft-wrap-aware copied text
- context-sensitive `Ctrl+C`: copy selection or send Unix `^C`
- `Ctrl+V` desktop-style paste
- automated VT core, PTY and interactive-shell smoke tests
- Fedora 44 GitHub Actions build/test workflow

Remaining M1 refinements:
- full grapheme-cluster handling for complex emoji/ZWJ/flags
- better horizontal resize/reflow of already wrapped history
- scrollback search and polished scrollbar UI
- more xterm/DEC modes and terminal query responses
- richer modified-key reporting
- more complete mouse protocol edge cases
- renderer performance refinement
- compatibility fixes discovered by `nano`, `vim`, `htop`, `btop`, `less`, `ssh`, `sudo`

M1 exit criterion: the compatibility probes should behave correctly enough that
AxiomTTY can serve as a normal interactive Linux terminal for everyday use.

## M2 — Sessions, tabs and splits — IN PROGRESS

Completed through M2.3:
- many simultaneous PTYs across tabs
- independent terminal state and scrollback per session
- inherited Linux working directory for new tabs/panes
- binary horizontal/vertical split tree with arbitrary nesting
- draggable split handles with independent PTY resizing
- active-pane tracking and keyboard pane navigation
- branch collapse when a pane is closed
- custom tab names with automatic-title reset
- right-click tab lifecycle menu and tab duplication
- duplicated tabs inherit shell + working directory
- active pane index/status (`PANE x/y`) and quieter inactive-pane focus UX

Remaining M2 work:
- pane duplication and richer clone/layout operations
- tab pin/reorder and drag between windows
- directional pane navigation based on geometry
- layout/session restore groundwork
- notifications for completed long-running commands

## M3 — Product-quality interaction
- searchable scrollback/history
- completion UI
- link/path detection
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


## M1 R6 – UI Foundation

- Custom dark client-side window chrome
- Unified header + active-session strip
- Native compositor move/resize for frameless window
- Quiet status bar
- Transient resize feedback
- Centralized AxiomTTY UI palette and metrics


## M1 R7 – Selection & Scrollback UX

- Selection is stored in history coordinates rather than viewport-only text
- Dragging near/outside the top or bottom edge auto-scrolls through history
- Auto-scroll speed increases with pointer distance beyond the edge
- Manual wheel/PageUp/PageDown scrolling no longer destroys an existing selection
- Ctrl+C can copy selections spanning many non-visible scrollback rows


## M1 R8 – TUI Compatibility

- Added standard/custom horizontal tab stop handling (HTS, TBC, CHT, CBT)
- Added CSI REP for repeated graphic characters
- Added SGR faint and strikethrough attributes
- Added xterm alternate-scroll mode 1007
- Added DEC synchronized-output mode 2026 and deferred GUI repaint while active
- Added OSC 10/11 foreground/background color query replies
- Improved DECSC/DECRC state preservation
- Added modifier-aware Insert/Delete/PageUp/PageDown and F1–F12 sequences
- Expanded smoke tests around the new VT behavior

## M2.1 — Tabs & Sessions

- Added a `SessionManager` model owning independent `TerminalSession` instances
- Each tab has its own PTY, terminal grid, process lifecycle and scrollback history
- New tabs inherit the active shell's current directory through `/proc/<pid>/cwd`
- Added compact multi-tab header UI with close controls and horizontal overflow
- Added standard tab shortcuts while preserving normal terminal `Ctrl+W`
- Added session count and active working directory to the status bar

Next M2 work: split panes backed by the same session infrastructure, followed by richer
tab lifecycle operations such as rename, reorder and duplicate.


## M2.2 — Split Panes

- Reworked each tab into a binary split tree whose leaves are real `TerminalSession`s
- Added arbitrarily nested horizontal and vertical splits
- Added split-right and split-down header controls and shortcuts
- Split panes inherit the active pane's `/proc/<pid>/cwd`
- Added draggable Qt Quick `SplitView` handles and per-pane PTY resizing
- Added active-pane tracking, next/previous pane shortcuts and pane count indicators
- Added branch collapse when closing a nested pane
- Added split-tree smoke coverage plus a dedicated manual regression checklist


## M2.3 — Tab/Pane & Session Polish

- Added editable custom tab titles (double-click or context menu)
- Added automatic-title reset so tabs can return to cwd/OSC-driven labels
- Added right-click tab lifecycle menu
- Added tab duplication preserving the active shell and `/proc/<pid>/cwd`
- Added active pane index tracking and `PANE x/y` status feedback
- Removed the persistent `CLICK TO TYPE` development badge from inactive panes
- Kept the terminal surface visually dominant with only subtle focus chrome
