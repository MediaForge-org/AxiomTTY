# M3.1 Settings Foundation — manual test

## 1. Natural last-pane / last-tab close

Start AxiomTTY with a single tab and a single pane.

- `Ctrl+Shift+X` should close that pane, close its tab, and then close AxiomTTY.
- Reopen AxiomTTY. `Ctrl+Shift+W` on the only idle tab should close AxiomTTY.
- With two tabs, closing the last pane of one tab should remove only that tab and activate the remaining tab.
- AxiomTTY must not silently create a fresh empty shell after the final pane/tab is closed.

With `sleep 60` running in the only pane, `Ctrl+Shift+X` should still ask for confirmation when close protection is enabled. `Cancel` keeps the process alive; `Close anyway` closes AxiomTTY.

## 2. Open settings

Open settings with either the gear button in the header or:

```text
Ctrl+,
```

The dialog should show Terminal and Sessions sections.

## 3. Live terminal font settings

Change Font size from 14 to 18. Existing panes should resize/reflow immediately and `stty size` should update after geometry settles.

Change Font family to another installed monospace family. Existing panes should update immediately.

## 4. Scrollback/history baseline

Scrollback retention is intentionally fixed internally for now instead of being exposed as a normal setting. Run:

```bash
seq 1 2000
```

Selection, search, and scrollback navigation should continue to work normally.

## 5. Default shell + start directory

Disable **New tabs inherit the active pane's working directory**.

Set:

```text
Default shell: /bin/bash
Start directory: /tmp
```

Open a new tab and run:

```bash
pwd
ps -p $$ -o comm=
```

The new tab should start in `/tmp`. The configured shell applies to newly created tabs; existing sessions are not replaced.

Re-enable working-directory inheritance, `cd` to the AxiomTTY repo, open a new tab, and verify that the new tab inherits that directory.

## 6. Close confirmation setting

With **Confirm before closing sessions with running child processes** enabled:

```bash
sleep 60
```

Then `Ctrl+Shift+X` should show the close confirmation dialog.

Disable the setting, run `sleep 60` again in a disposable pane, and close the pane. It should close immediately without the dialog.

Re-enable the protection after the test.

## 7. Persistence

Close AxiomTTY normally, reopen it, and open Settings again. Values should persist in:

```text
~/.config/axiomtty/settings.ini
```

(`XDG_CONFIG_HOME` is respected when configured.)

Use **Reset defaults** to restore the initial settings and verify that the controls update immediately.


## M3.1.1 reliability checks

- Enter `/tmp` in **Start directory**: a new tab starts in `/tmp` with the configured shell.
- Enter `/tmp` in **Default shell**: AxiomTTY rejects the directory and falls back to a valid shell.
- Press **Reset defaults** repeatedly: the dialog stays responsive; Close still works.
- Split then close a pane repeatedly: resizing must not produce duplicate prompt redraws from duplicate SIGWINCH delivery.
- Scrollback retention is fixed internally for now and is no longer exposed in Settings.
