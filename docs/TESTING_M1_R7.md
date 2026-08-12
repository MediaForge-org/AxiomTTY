# AxiomTTY M1 R7 — Selection & Scrollback manual test

R7 focuses on selecting and copying terminal output beyond one visible viewport.

## 1. Produce enough history

```bash
seq 1 1000
```

## 2. Drag upward through scrollback

1. Start a selection near the bottom of the terminal.
2. Keep the left mouse button held.
3. Drag to the top edge and continue slightly above it.
4. AxiomTTY should automatically scroll upward while extending the selection.
5. Moving farther above the edge should scroll faster.
6. Drag back toward the center to stop auto-scroll.

Repeat in the opposite direction while scrolled back: drag to/below the bottom edge
and verify that the viewport auto-scrolls toward the newest output.

## 3. Copy many pages

Select a range spanning several screenfuls and press `Ctrl+C`.
Paste into an editor or run `wc -l` on the pasted content. The copied text must
include non-visible lines between the selection endpoints, not only the current viewport.

## 4. Selection survives manual scrolling

1. Make a selection.
2. Scroll with the mouse wheel, `Shift+PageUp`, and `Shift+PageDown`.
3. The selection must remain active even if it is partly or entirely off-screen.
4. `Ctrl+C` must still copy the original complete range.

## 5. Unix Ctrl+C behavior remains intact

Clear the selection, then run:

```bash
sleep 60
```

Press `Ctrl+C`. The process must be interrupted normally.

## 6. TUI mouse ownership

If `vim`, `htop`, or another application enables mouse reporting, normal mouse events
belong to that application. Hold **Shift** while dragging to force a local AxiomTTY
selection, including edge auto-scroll when primary scrollback is available.
