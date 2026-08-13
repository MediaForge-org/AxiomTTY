# M2.2 manual test — Split panes

M2.2 changes a tab from a single terminal session into a binary split tree. Every leaf
is a real `TerminalSession` with its own PTY, process, screen buffer and scrollback.

## 1. Split right

Start AxiomTTY and press `Ctrl+Shift+D` or click the vertical split icon in the header.
The active terminal should split into two side-by-side panes.

Run in the left pane:

```bash
printf 'LEFT\n'
sleep 60
```

Click the right pane and run:

```bash
printf 'RIGHT\n'
pwd
```

The sleeping process in the left pane must not block the right pane.

## 2. Split down

Focus either pane and press `Ctrl+Shift+E`. Only the focused pane should split into a
top/bottom pair. This should create a nested layout rather than replacing the whole tab.

Create several combinations, for example:

```text
+--------------------+--------------------+
|                    |        top         |
|       left         +--------------------+
|                    |       bottom       |
+--------------------+--------------------+
```

Then split one of those leaves again. Nested mixed horizontal/vertical splits should
remain stable.

## 3. Working-directory inheritance

In an active pane:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
pwd
```

Wait briefly, then split that pane. In the newly created pane:

```bash
pwd
```

It should start in the same directory.

## 4. Resize split handles

Drag the separator between panes. Both terminal PTYs should receive their new cell sizes.
Verify in each pane with:

```bash
stty size
```

The values should change independently as the split handle moves.

## 5. Active pane and focus

The active pane has a subtle AxiomTTY accent marker. Clicking a different terminal must
move the active state there.

Test keyboard pane navigation:

```text
Ctrl+Shift+Right     next pane
Ctrl+Shift+Left      previous pane
```

Input must always go only to the active pane.

## 6. Close panes

Create at least four panes. Focus a middle/nested pane and press:

```text
Ctrl+Shift+X
```

Only that pane should close. Its sibling should take over the freed space and the rest of
the split tree must remain intact.

Closing the last pane in a tab behaves like closing that tab. If it was the final tab,
AxiomTTY creates a fresh shell as before.

`Ctrl+Shift+W` still closes the whole tab, including all panes in it.

## 7. Tabs containing split layouts

Create a complex split layout in tab 1, then press `Ctrl+Shift+T`. Tab 2 should start as a
single independent pane. Switch between the tabs repeatedly. Tab 1 must retain its split
layout and running programs.

The tab label shows a small pane count when a tab contains more than one pane.

## 8. TUI stress test

Try a layout such as:

- pane 1: `btop`
- pane 2: `htop`
- pane 3: `nano /tmp/axiomtty-split.txt`
- pane 4: ordinary Bash commands

Resize the window and the split handles, switch tabs and focus panes. Look for mixed
output, wrong cursor placement, stale frames, crashes or input going to the wrong PTY.

## 9. Regression checks

Verify that M1/M2.1 behavior still works:

- scrollback and cross-history selection
- context-sensitive `Ctrl+C`
- `Ctrl+V`
- `less`, `nano`, `vim`, `htop`, `btop`
- tab creation/closing/navigation
- new tab working-directory inheritance
- normal terminal `Ctrl+W`
