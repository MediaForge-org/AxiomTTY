# AxiomTTY M3.1.2 — Font Resize Stability

This hotfix targets the visible terminal-content jump that occurred when changing the live terminal font size.

## Primary regression test

1. Start AxiomTTY with one pane.
2. Produce a few prompts so there is both visible content and some history:

```bash
echo one
echo two
echo three
seq 1 80
clear
printf "ANCHOR\n"
```

3. Open Settings with `Ctrl+,`.
4. Change font size repeatedly, for example `14 -> 18 -> 12 -> 20 -> 14`.
5. The current visible content must stay on the same logical terminal rows. Older scrollback must not be pulled back into the live viewport just because a smaller font creates more rows.
6. The cursor/prompt must remain attached to the same logical content instead of visibly walking down/up through history.

## Scrollback anchor test

```bash
seq 1 500
```

Scroll upward several pages, note a visible line number, then change font size. The viewport should remain anchored around the same history location and must not jump to unrelated recent/older content.

## Split test

Create 3-5 nested panes, put a prompt in each, and change the font size. Every pane should resize independently without importing old history into its current screen.

## TUI sanity test

Run `btop`, `htop`, `nano`, or `vim` in a pane and change the font size. The application may redraw after SIGWINCH, but AxiomTTY itself must not inject historical rows into the live screen.

## Existing behavior that must remain

Normal window/split resizing keeps the existing bottom-preserving terminal behavior. The new top-preserving policy is used specifically for font-metric changes.
