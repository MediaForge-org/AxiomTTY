# M3.2.7 Pane close and focus polish

## Explicit close target

1. Create 4-8 panes.
2. Hover a non-active pane. A small close button should appear in its top-right corner.
3. Click the close button. Exactly that pane must disappear.
4. The remaining panes must rebalance immediately.

## Keyboard close

1. Click/type in a pane so its accent border is visible.
2. Press `Ctrl+Shift+X`.
3. Exactly the accented pane must close.
4. Focus should move to the nearest pane across the removed split boundary.

## Busy process

Run `sleep 60` in a pane, then close it via the pane close button. The same close confirmation used by `Ctrl+Shift+X` must appear. Cancel must keep the exact pane alive; confirm must close that exact pane even if focus changes while the dialog is open.

## Balance

Create 8 horizontal panes, then close pane 3 and pane 6. The remaining six panes should be evenly distributed across the width. Repeat vertically.
