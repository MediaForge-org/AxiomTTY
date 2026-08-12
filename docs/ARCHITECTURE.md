# Architecture

## Direction

TerminalCpp is Linux-first. Fedora is the first reference platform. Linux/POSIX
semantics define behavior; future ports adapt to that model rather than reducing
the design to a lowest common denominator.

## Layering

```text
QML chrome / interaction
  |
C++ TerminalView renderer
  |
TerminalSession
  |
VT parser -> TerminalScreen cell grid       Own shell (later)
  |                                         |-- lexer/parser
PTY/process layer                           |-- executor/job control
  |                                         |-- built-ins
Linux/POSIX                                 `-- completion/history
```

The GUI does not own process semantics. The terminal emulator does not know
about future workspaces or package-manager UI. The future own shell remains a
separate component and can later be built as a standalone executable.

## Current milestone — M1 Terminal Core

Implemented:

- Qt Quick/QML application shell
- native Linux PTY using `forkpty()`
- real user shell from `$SHELL`
- non-blocking PTY reads in Qt's event loop
- process-group signalling and resize propagation
- C++ terminal cell grid
- incremental UTF-8 decoder
- VT/ECMA-48 parser foundation
- cursor positioning, scrolling, insert/delete/erase primitives
- SGR 16-color, 256-color and true-color parsing
- bold/italic/underline/inverse cell attributes
- primary + alternate screen buffers
- application cursor-key, cursor-visible and bracketed-paste modes
- OSC terminal title handling
- terminal query responses needed by interactive software
- C++ `QQuickPaintedItem` renderer

Installed Linux commands continue to be real executables. `git`, `dnf`, `sudo`,
`ssh`, `docker`, `cmake`, etc. are not reimplemented.

## Still incomplete inside M1

- Unicode display-width rules (wide CJK/emoji and combining marks)
- scrollback navigation/search (rows are collected internally only)
- native cell selection with context-sensitive Ctrl+C copy / Unix interrupt
- mouse reporting protocols
- complete xterm/DEC private-mode coverage
- optimized scene-graph renderer for extreme output rates
- accessibility and IME refinement

`QQuickPaintedItem` is intentionally an early renderer. Once terminal semantics
are stable, a lower-level Qt Quick scene-graph renderer can replace it without
changing PTY, parser or screen-model architecture.
