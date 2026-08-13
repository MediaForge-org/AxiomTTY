# M2.5 Pane Navigation & Lifecycle Test

M2.5 focuses on pane navigation, pane duplication and safer session lifecycle behavior.

## 1. Directional pane navigation

Create this layout:

```text
+-------------------+-------------------+
|                   |       pane B      |
|      pane A       +-------------------+
|                   |   pane C | pane D |
+-------------------+-------------------+
```

Use:

```text
Ctrl+Shift+Left
Ctrl+Shift+Right
Ctrl+Shift+Up
Ctrl+Shift+Down
```

Focus should move in the requested geometric direction and should stay where it is
when no pane exists in that direction.

## 2. Duplicate active pane

In a pane:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
printf 'source shell: %s\n' "$SHELL"
```

Use:

```text
Alt+Shift+D   duplicate to the right
Alt+Shift+E   duplicate downward
```

The new pane must have its own PTY/process while inheriting the active pane's shell
and current working directory.

## 3. Close protection for a busy pane

Run:

```bash
sleep 60
```

Then press:

```text
Ctrl+Shift+X
```

A confirmation popup must appear. `Cancel` keeps the process and pane alive.
`Close anyway` removes only that pane.

A pane containing only an idle shell should close immediately without a popup.

## 4. Close protection for a busy tab

Create two panes in one tab. Run `sleep 60` in either pane, then press:

```text
Ctrl+Shift+W
```

The whole-tab close must ask for confirmation because one pane has a child process.

## 5. Window close protection

Run `sleep 60` or `btop` in any pane, then click the AxiomTTY window close button.
The application must ask before closing. With only idle shells, closing the window
should happen immediately.

## 6. Regression checks

Verify that these still work:

- `Ctrl+F` scrollback search
- selection across scrollback and `Ctrl+C`
- `Ctrl+V` paste
- `Ctrl+Shift+D/E` normal splits
- tab rename and duplicate tab
- `btop`, `htop`, `nano`, `vim`/`less`
