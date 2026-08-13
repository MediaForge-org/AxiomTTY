# AxiomTTY M2.3 manual test

M2.3 is a lifecycle/UX pass on top of the M2.2 split-tree architecture.

## 1. Regression baseline

- Start AxiomTTY.
- Create nested splits with `Ctrl+Shift+D` and `Ctrl+Shift+E`.
- Verify each pane remains an independent PTY.
- Verify `Ctrl+C` interrupts only the focused pane when no selection exists.
- Verify selection copy, paste, scrollback and TUI programs still work.

## 2. Quiet inactive panes

Create at least three panes and focus them one after another. Inactive panes must no longer show a persistent `CLICK TO TYPE` badge. The active pane should still be identifiable by the subtle accent border/edge.

## 3. Active pane status

Create five panes. The bottom status bar should show `PANE 1/5`, `PANE 2/5`, etc. as focus moves with mouse clicks or `Ctrl+Shift+Left/Right`.

## 4. Rename tab

Double-click a tab title, enter `AxiomTTY Dev`, and press Enter. The label must stay `AxiomTTY Dev` even when the active pane changes directories or changes its OSC title.

Press Escape while editing another rename: the previous title must be kept.

## 5. Reset automatic title

Right-click the renamed tab and choose **Use Automatic Title**. The label should return to the cwd/OSC-derived automatic title.

## 6. Duplicate tab

In a tab run:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
pwd
```

Right-click the tab and choose **Duplicate Tab**. In the new tab:

```bash
pwd
echo "$SHELL"
```

The working directory should match the source tab and the same shell executable should be launched, while the new PTY remains independent.

## 7. Independence after duplication

Source tab:

```bash
sleep 60
```

Duplicated tab:

```bash
echo independent
```

The duplicated tab must remain responsive. Interrupting one tab must not interrupt the other.

## 8. Context menu lifecycle

Right-click tabs and exercise **Duplicate Tab**, **Rename Tab**, **Use Automatic Title**, and **Close Tab**. Middle-click close and the normal close button must still work.

## Rename hotfix
- Double-click a tab: a rename popup must open below the tab.
- Type a name and press Enter: the tab title must change immediately.
- Press Escape or click outside: the rename popup closes without changing the title.
- Right-click -> Rename Tab must open the same popup reliably.
