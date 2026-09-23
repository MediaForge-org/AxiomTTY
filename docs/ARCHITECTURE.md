# Architecture

## Direction

AxiomTTY is Linux-first. Fedora is the first reference platform. Linux/POSIX
semantics define behavior; future ports should adapt to that model rather than
reducing the design to a lowest common denominator.

## Layering

```text
QML chrome / interaction
  |
SessionManager (tabs + active pane)
  |
SplitNode tree -> TerminalView leaves <-> TerminalSession
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

## Current milestone — M3.2 Themes & Profiles

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
- pane-local search mapped back onto terminal history cells
- highlighted current/all search results with history navigation
- split-tree directional pane neighbour resolution
- child-process-aware pane/tab/application close requests
- fresh-PTY pane duplication with shell/CWD inheritance
- persistent `AppSettings` service backed by an XDG config INI file
- persistent profile registry with shell/start-directory/color-scheme data
- per-session profile identity and terminal palette selection
- live scrollback-capacity propagation to existing sessions
- settings-driven new-tab shell/start-directory behavior
- natural final-pane/final-tab application close semantics

Installed Linux commands continue to be real executables. `git`, `dnf`, `sudo`,
`ssh`, `docker`, `cmake`, etc. are not reimplemented.

### Session and split ownership

`SessionManager` owns tabs and all `TerminalSession` objects. Each tab owns a binary
`SplitNode` tree. Every leaf of that tree points at one independent `TerminalSession`,
and every `TerminalSession` owns one PTY, one VT parser and one terminal screen model.
Switching tabs or panes never recreates or pauses the underlying processes.

Splitting a leaf mutates that leaf into a branch and creates two child leaves: the
existing session and a newly created session. Horizontal branches render side-by-side;
vertical branches render top/bottom. Because branches can contain further branches, the
layout supports arbitrary nested combinations without a separate layout subsystem.

When a pane closes, its sibling is promoted into the parent node. This collapses only
the affected branch and preserves the rest of the tree. The tab tracks one active leaf
for keyboard focus, status information and explicit CWD-preserving duplicate/split actions.

Fresh tabs resolve their working directory from the selected profile. Duplicate Tab and
split/duplicate pane operations query `/proc/<shell-pid>/cwd` when preserving the active
work context. This keeps profile launches deterministic while avoiding prompt-text heuristics.

## Still incomplete inside M1

- full grapheme clusters for complex emoji/ZWJ/flags
- horizontal reflow of existing wrapped history after width changes
- polished scrollbar UI
- complete xterm/DEC private-mode and query coverage
- richer modified-key reporting
- remaining mouse protocol edge cases
- optimized scene-graph renderer for extreme output rates
- accessibility and IME refinement

`QQuickPaintedItem` is intentionally an early renderer. Once terminal semantics
are stable, a lower-level Qt Quick scene-graph renderer can replace it without
changing PTY, parser or screen-model architecture.


## M2.5 navigation and lifecycle notes

`SplitNode` now resolves directional pane neighbours from the split-tree topology.
`SessionManager` owns close requests and only asks the UI for confirmation when the
PTY shell currently has child processes. Plain idle shells retain immediate close
behavior. Pane duplication creates a fresh PTY using the active pane's shell and CWD;
it does not clone process memory or terminal scrollback.


## Settings ownership

`AppSettings` is a C++ application service exposed to QML as `appSettings`. It owns persistent product configuration and writes to the XDG configuration root under `axiomtty/settings.ini`. QML presents and edits those values; terminal/process semantics remain in C++.

Font settings bind directly to each `TerminalView`, so existing panes update live. Scrollback capacity is propagated by `SessionManager` into every existing `TerminalSession`/`TerminalScreen`. Shell and start-directory settings intentionally affect newly created tabs rather than replacing already-running PTYs.

The final-pane lifecycle is also owned by `SessionManager`: removing the only pane removes the tab, and removing the final tab emits application-close approval. The manager never creates an implicit replacement shell as a side effect of closing.


## Profile and color-scheme ownership

`AppSettings` owns the persistent profile registry. `SessionManager` chooses a profile when a tab is created and stamps that identity onto every `TerminalSession` in the tab. New split panes inherit the tab profile; they do not silently switch to the global default profile.

A profile controls startup policy (shell and directory) and presentation policy (terminal color scheme). Changing a profile's shell or directory affects future PTYs. Changing its color scheme is safe to propagate to existing sessions because it only changes rendering.

`TerminalView` owns color-scheme rendering. The terminal screen model continues to store canonical default/base ANSI colors and arbitrary 256/True Color values. At paint time the renderer remaps the canonical default and 16-color ANSI palette through the selected scheme. Arbitrary True Color values remain untouched. This keeps VT state independent from product theming and allows existing scrollback to recolor immediately without rewriting terminal history.


## M3.2.1 settings event flow

The Settings dialog no longer treats every profile field edit as a global profile mutation. Shell, start-directory and color-scheme values are staged in QML and committed through one `setProfileSettings()` call. `AppSettings` persists that profile once, while color changes emit a narrow `profileColorSchemeChanged(profile, scheme)` signal. `SessionManager` updates only tabs that actually use that profile. This prevents the old feedback loop where profile edits triggered a complete Settings resync plus a repaint of unrelated sessions.

Font-size changes remain live but are debounced briefly in the UI so rapid SpinBox interaction produces one terminal-grid resize after the user pauses rather than a resize for every intermediate event.

Fresh tabs are deliberately profile-defined: `+`, `Ctrl+Shift+T`, and explicit profile selection always use the target profile's configured start directory. CWD preservation is explicit: Duplicate Tab and split/duplicate pane actions preserve the active session's `/proc/<pid>/cwd`.
