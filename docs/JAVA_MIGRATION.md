# Java prototype migration notes

The uploaded Java prototype contains a single `MyTerminal.java` of roughly 1300
lines. Its command switch includes these command names:

`help`, `clear`, `ls`, `cd`, `mkdir`, `mkdirhier`, `touch`, `rmdir`, `rm`, `cp`,
`mv`, `cat`, `pwd`, `tree`, `ifconfig`, `shutdown`, `zip`, `unzip`, `echo`, `ps`,
`kill`, `grep`, `wc`.

## What is retained

- Linux-style command naming and prompt concept
- the ambition to expose many terminal commands
- useful ideas such as history/help/tree/file operations

## What is intentionally not ported line-by-line

- the single giant command switch
- parsing through `command.split(" ")`
- duplicated implementations of standard Linux utilities
- Windows-specific `tasklist`, `taskkill`, WinRAR paths and shutdown behavior
- direct coupling between terminal UI, command parsing and OS operations

## New strategy

Standard Linux tools remain the real system tools. For example `grep` resolves to
the installed `grep`; `dnf` is the real Fedora package manager; `git` is the real
Git executable. The own shell will implement only shell semantics and true
built-ins, plus deliberate TerminalCpp extensions.
