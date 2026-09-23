# M3.2.4 Settings responsiveness test

1. Open Settings with `Ctrl+,`.
2. Edit Default start directory from `/tmp` to `/home`, press Apply profile, then immediately click other controls. The UI must remain responsive.
3. Open a new Default tab and run `pwd`; it must use `/home` immediately even though disk persistence has not been flushed yet.
4. Change the path several times (`/tmp`, `/home`, `~`) and apply each change. No click should freeze the dialog or terminal.
5. Close AxiomTTY normally, restart it, and confirm the last applied value persisted.
6. Repeat theme/profile changes while multiple panes are open; profile recoloring must remain targeted and the Settings UI must stay interactive.
