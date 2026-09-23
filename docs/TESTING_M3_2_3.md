# M3.2.3 — New-tab start-directory semantics

## 1. Fresh tab respects profile start directory

Set the active/default profile start directory to `/home` and press **Apply profile**. In the current tab, move elsewhere:

```bash
cd /tmp
pwd
```

Create a fresh tab with `+` or `Ctrl+Shift+T`, then:

```bash
pwd
```

Expected: `/home`. The current pane's `/tmp` must not leak into a fresh tab.

## 2. Explicit profile selection

Set Development to `/tmp` and Default to `/home`. Right-click `+` and open each profile directly. `pwd` must match that profile's configured start directory every time.

## 3. Duplicate Tab preserves context

In a tab, `cd` to the AxiomTTY repository, then use **Duplicate Tab** from the tab context menu. The duplicate must have a fresh PTY/process but `pwd` should match the source tab.

## 4. Splits preserve context

In the same repository directory, create a right/down split or duplicate pane. `pwd` in the new pane should match the active pane while still using an independent PTY.

## 5. Settings clarity

Settings must no longer show the old "New tabs inherit the active pane's working directory" toggle. The explanatory text should state that fresh tabs use profile start directories while duplication/splits preserve CWD.
