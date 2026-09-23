> **Superseded by M3.2.3 for new-tab CWD behavior:** use `docs/TESTING_M3_2_3.md` for current semantics.

# M3.2 — Themes & Profiles manual test

## 1. Built-in profiles

Open Settings with `Ctrl+,` and verify these profiles exist:

- Default
- Development
- Server

Expected default color schemes:

- Default → Axiom Dark
- Development → Midnight
- Server → Graphite

## 2. Live terminal color scheme

Open two tabs with the Development profile. In Settings, edit Development and change
its color scheme between Midnight, Forest, Graphite and Axiom Dark.

Expected:

- all already-open panes using Development update immediately
- running processes continue uninterrupted
- scrollback and selection remain intact
- True Color output remains True Color while the base 16 ANSI colors follow the scheme

Useful color probe:

```bash
printf '\e[30m00 \e[31m01 \e[32m02 \e[33m03 \e[34m04 \e[35m05 \e[36m06 \e[37m07\e[0m\n'
printf '\e[90m08 \e[91m09 \e[92m10 \e[93m11 \e[94m12 \e[95m13 \e[96m14 \e[97m15\e[0m\n'
printf '\e[38;2;255;100;30mTRUECOLOR\e[0m\n'
```

## 3. Active/default profile

Set `Default for new tabs` to Development. Disable CWD inheritance temporarily.
Create a normal new tab with `Ctrl+Shift+T`.

Expected:

- status bar shows DEVELOPMENT
- shell/start directory come from Development
- terminal uses the Development color scheme

## 4. Open a specific profile

Right-click the `+` button in the header.

Expected: menu lists all profiles.

Open Server from the menu.

Expected:

- new independent PTY
- status bar shows SERVER
- Server shell/start directory/theme are used

## 5. Profile shell and directory

Set Server:

- shell: `/bin/bash`
- start directory: `/tmp`

Turn off CWD inheritance and create Server from the `+` context menu.

```bash
pwd
ps -p $$ -o comm=
```

Expected: `/tmp` and `bash`.

## 6. Custom profile persistence

Create profile `Test Profile`, set:

- shell `/bin/sh`
- start directory `/tmp`
- color scheme Forest

Set it as the default profile for new tabs. Restart AxiomTTY.

Expected: profile and all values persist.

Then remove the custom profile. Built-in profiles must not be removable.

## 7. Split inheritance

Open a Development tab and split it several times.

Expected: every pane in that tab keeps the Development profile and theme. Changing
Development's color scheme should update every pane in that tab live.

## 8. Regression

Recheck:

- `Ctrl+F` scrollback search
- copy/paste and long-history selection
- font-size changes remain viewport-stable
- `btop`, `htop`, `nano`, `less`
- close confirmation with `sleep 60`
- Reset defaults remains responsive
