# M3.1.3 Settings Interaction Reliability

## Reset button stress test

1. Open Settings with `Ctrl+,`.
2. Change font size, font family, start directory and both switches.
3. Click `Reset defaults` ten times, waiting for `Resetting…` to return to `Reset defaults` between clicks.
4. Every click must react immediately; the window must remain responsive.
5. `Tab`, `Shift+Tab`, mouse clicks and the Close button must remain responsive after every reset.
6. Close and reopen Settings; defaults must still be shown.
7. Restart AxiomTTY and verify defaults persisted.

## Rapid settings edits

Change font size repeatedly and toggle switches quickly. The settings dialog must not freeze or stop accepting clicks.
