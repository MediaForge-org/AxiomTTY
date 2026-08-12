# TerminalCpp — M1 Terminal Core R4 (Pre-Alpha)

TerminalCpp is a Linux-first terminal application being rebuilt in C++23 and Qt 6/QML.
The project is intentionally still **pre-alpha**; milestone names are used instead of a public semantic version.

## M1 adds

- real Linux PTY process execution
- direct keyboard input to the PTY
- C++ terminal cell grid (no QML TextArea as the terminal surface)
- incremental UTF-8 decoding
- ANSI/VT CSI parser foundation
- cursor movement and screen erase operations
- 16-color, 256-color and True Color SGR parsing
- bold, italic, underline and inverse attributes
- primary and alternate screen buffers
- cursor visibility mode
- application cursor-key mode
- bracketed paste mode
- terminal title handling through OSC 0 / OSC 2
- basic terminal status/device responses
- C++ QQuickPaintedItem renderer
- automatic PTY resize based on the rendered cell geometry
- VT core smoke test
- mouse drag text selection on the visible terminal grid
- `Ctrl+C` copies when text is selected; otherwise it sends the normal Unix `^C` interrupt
- `Ctrl+Shift+C` remains available as an optional copy alias
- `Ctrl+V` pastes from the clipboard (`Ctrl+Shift+V` remains an alias)
- 5000-line scrollback buffer can now be viewed with the mouse wheel or `Shift+PageUp` / `Shift+PageDown`
- selection and copy also work while viewing visible scrollback rows

## Fedora build

```bash
./scripts/install-fedora.sh   # only needed once
./scripts/build.sh
./build/terminal_cpp
```

The build script also runs `ctest` when tests are enabled.

## Useful M1 tests inside the terminal

```bash
printf '\e[31mRED\e[0m  \e[32mGREEN\e[0m  \e[38;2;120;180;255mTRUECOLOR\e[0m\n'
ls --color=always
clear
printf '\e]2;Custom terminal title\a'
```

Try `nano`, `vim`, `htop` or `btop` as compatibility probes. M1 implements enough terminal primitives for them to become meaningful tests, but full xterm/VT compatibility, terminal mouse-reporting protocols, Unicode cell-width handling, soft-wrap-aware selection and many edge cases are still future milestones.

## Design direction

Linux/POSIX is the reference behavior. The GUI is deliberately restrained and desktop-oriented rather than card-heavy, gradient-heavy or AI/SaaS styled.
