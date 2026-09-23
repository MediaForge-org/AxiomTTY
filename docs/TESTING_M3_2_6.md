# M3.2.6 Balanced pane layout test

## Equal-width repeated splits

1. Start with one pane.
2. Press `Ctrl+Shift+D` repeatedly until there are 8 panes.
3. All 8 panes must have approximately the same width. The layout must not form a `1/2, 1/4, 1/8...` cascade.
4. Close any middle pane with `Ctrl+Shift+X`.
5. The remaining 7 panes must redistribute evenly.

## Equal-height repeated splits

1. Start a fresh tab.
2. Press `Ctrl+Shift+E` repeatedly until the pane limit is reached.
3. All panes must have approximately the same height.

## Mixed grid

1. Split right once.
2. Split the right pane down.
3. Split the lower-right pane right.
4. Horizontal siblings should share width according to their logical columns, and vertical siblings should share height according to their logical rows.

## Pane close visual cleanup

1. Create three panes.
2. Type a unique marker such as `CLOSED-PANE-123` in one pane.
3. Close that pane.
4. The complete visual split tree is rebuilt; the marker from the removed pane must disappear immediately.
5. The surviving pane may expand, but expanding it must not pull old scrollback lines back onto the live screen as apparent ghost content.

## Limit

- A tab still stops at 8 panes.
- Closing one pane makes splitting available again.
