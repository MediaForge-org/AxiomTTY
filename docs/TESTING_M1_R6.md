# AxiomTTY M1 R6 – UI Foundation test notes

R6 is primarily a visual/desktop-integration pass. Terminal-core behavior from R5 must remain intact.

## Window chrome

- The native bright GNOME/Fedora title bar should be gone.
- AxiomTTY should draw one compact dark header containing the product name, active session tab and window controls.
- Drag the empty header area: the window should move using the compositor.
- Double-click the empty header area: maximize/restore.
- Minimize, maximize/restore and close buttons should work.
- Resize from all four edges and all four corners.
- Maximize the window and verify the custom outer border disappears.

## Terminal regression

Run:

```bash
printf '\e[31mRED\e[0m  \e[32mGREEN\e[0m  \e[38;2;120;180;255mTRUECOLOR\e[0m\n'
seq 1 300
sleep 30
```

Verify:

- ANSI colors still render.
- Scrollback still works with the mouse wheel and Shift+PageUp/PageDown.
- Mouse selection still works.
- Ctrl+C copies when a selection exists.
- Ctrl+C interrupts `sleep 30` when no selection exists.
- Ctrl+V pastes.

## Resize indicator

Resize the window. The `columns × rows` badge should appear briefly and then disappear. It should not be permanently visible during normal terminal use.

## Status bar

The bottom bar should be quiet: shell name on the left, UTF-8 on the right, no PID and no permanent RUNNING debug label.
