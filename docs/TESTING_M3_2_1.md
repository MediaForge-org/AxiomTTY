> **Superseded by M3.2.3 for new-tab CWD behavior:** use `docs/TESTING_M3_2_3.md` for current semantics.

# AxiomTTY M3.2.1 — Profiles & Settings Reliability

## 1. Settings responsiveness

Open Settings with `Ctrl+,`. Move through the profile selector, change shell text, start directory and color scheme repeatedly. The terminal UI must remain responsive. Profile changes are staged until **Apply profile** or **Close**.

## 2. Theme application

Use a tab whose status bar says `DEFAULT`. Edit **Default**, choose each scheme and press **Apply profile**:

- Axiom Dark — neutral near-black
- Midnight — visibly blue-black
- Graphite — visibly neutral gray
- Forest — visibly green-black

The existing tab must recolor without restarting its shell or losing scrollback.

Run:

```bash
printf '\e[31mRED\e[0m \e[32mGREEN\e[0m \e[34mBLUE\e[0m \e[38;2;255;100;30mTRUECOLOR\e[0m\n'
```

The ANSI colors may change with the scheme; `TRUECOLOR` must remain the explicit RGB color.

## 3. Profile start directory with CWD inheritance enabled

Keep CWD inheritance enabled. In a Default tab, set Development to `/tmp` and make Development the default for new tabs. `Ctrl+Shift+T` must open a Development tab in `/tmp`, because switching profiles uses the target profile start directory.

Then in that Development tab:

```bash
cd /mnt/Festplatte/Schreibtisch/Projekte/AxiomTTY
```

Open another tab with `Ctrl+Shift+T`. Because it stays on Development, the new tab should inherit the project directory.

## 4. Font-size responsiveness

Rapidly adjust font size several steps. AxiomTTY should update after a short pause without freezing or processing every intermediate resize. The viewport-stability behavior from M3.1.2 must remain intact.

## 5. Persistence

Close AxiomTTY and reopen it. Applied profile values, the default profile and general Settings values must persist.
