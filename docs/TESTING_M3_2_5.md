# M3.2.5 Pane UX reliability test

## Closed pane must disappear visually

1. Create a right split and then split the right pane downward.
2. Type a unique marker such as `CLOSED-PANE-123` in the lower-right pane.
3. Close that pane with `Ctrl+Shift+X`.
4. The removed pane and its marker must disappear immediately. The surviving pane may expand, but no stale painted content from the removed pane may remain.

## Settings toggle

- `Ctrl+,` opens Settings.
- Pressing `Ctrl+,` again closes Settings.
- `Escape` closes Settings.
- The Close button still closes Settings.

## Pane limit

- Repeatedly split with `Ctrl+Shift+D` / `Ctrl+Shift+E`.
- A tab stops at 8 panes.
- Split toolbar buttons become disabled at the limit.
- Keyboard shortcuts cannot bypass the limit.
- Closing one pane enables splitting again.
