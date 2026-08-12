# Architecture

## Direction

AxiomTTY is Linux-first. Fedora is the first reference platform. Linux/POSIX
semantics define behavior; future ports should adapt to that model rather than
reducing the design to a lowest common denominator.

## Layering

```text
QML chrome / interaction
  |
SessionManager (tabs; split tree next)
  |
TerminalView <-> active TerminalSession
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

## Current milestone — M2.1 Tabs & Sessions

Implemented:

- Qt Quick/QML application shell
- native Linux PTY using `forkpty()`
- real user shell from `$SHELL`
- non-blocking PTY reads in Qt's event loop
- resize propagation and terminal-size reporting
- C++ terminal cell grid and 5000-line scrollback
- incremental UTF-8 decoder
- single-width, double-width and combining-mark cell handling
- soft-wrap metadata for copied text
- VT/ECMA-48 parser foundation
- cursor positioning, scrolling, insert/delete/erase primitives
- SGR 16-color, 256-color and True Color parsing
- bold/italic/underline/inverse cell attributes
- primary + alternate screen buffers
- DEC Special Graphics line drawing
- block/underline/bar cursor styles with blink control
- application cursor-key, origin, insert and bracketed-paste modes
- xterm mouse tracking foundations and SGR mouse reporting
- xterm focus reporting
- OSC terminal title handling
- basic terminal query responses
- C++ `QQuickPaintedItem` renderer
- native mouse selection and context-sensitive `Ctrl+C`
- direct `Ctrl+V` paste and bracketed-paste support

Installed Linux commands continue to be real executables. `git`, `dnf`, `sudo`,
`ssh`, `docker`, `cmake`, etc. are not reimplemented.

### Session ownership

`SessionManager` is now the application-level owner of terminal sessions. Each
`TerminalSession` owns one PTY, one VT parser and one terminal screen model. Switching
tabs only rebinds the `TerminalView` to another session; it does not recreate or pause
the underlying process.

New tabs query `/proc/<shell-pid>/cwd` and start in the active shell's current directory.
This keeps Linux shell semantics while avoiding any attempt to infer a directory from
prompt text. The same session objects are intended to become leaves in the M2 split
tree, so tabs and splits will share one lifecycle model rather than separate hacks.

## Still incomplete inside M1

- full grapheme clusters for complex emoji/ZWJ/flags
- horizontal reflow of existing wrapped history after width changes
- scrollback search and polished scrollbar UI
- complete xterm/DEC private-mode and query coverage
- richer modified-key reporting
- remaining mouse protocol edge cases
- optimized scene-graph renderer for extreme output rates
- accessibility and IME refinement

`QQuickPaintedItem` is intentionally an early renderer. Once terminal semantics
are stable, a lower-level Qt Quick scene-graph renderer can replace it without
changing PTY, parser or screen-model architecture.
