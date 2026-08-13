# AxiomTTY M2.4 — Scrollback Search Test

M2.4 adds pane-local search over the complete terminal history.

## 1. Build

```bash
./scripts/build.sh
./build/axiomtty
```

The VT smoke test now also verifies case-sensitive/case-insensitive search,
soft-wrapped matches and wide-character coordinates.

## 2. Populate history

```bash
for i in $(seq 1 1500); do
    printf 'line %04d  alpha beta %s\n' "$i" "$([ $((i % 125)) -eq 0 ] && echo ERROR || echo ok)"
done
```

## 3. Open search

Press `Ctrl+F` in the active pane. A compact search bar should appear in the
upper-right corner of that pane only.

Search for:

```text
ERROR
```

Expected:
- count is shown as `current / total`
- all visible matches are highlighted
- the current match is highlighted more strongly
- the viewport jumps into scrollback to show the current result

## 4. Navigate

- `Enter` or `F3`: next match
- `Shift+Enter` or `Shift+F3`: previous match
- arrow buttons in the search bar do the same
- navigation wraps from last to first and first to last

## 5. Case sensitivity

Search for `error`, then toggle `Aa`.

With case sensitivity enabled, lowercase `error` must not match uppercase
`ERROR` from the generated output.

## 6. Soft wraps

Make a narrow pane and run:

```bash
printf 'prefix-ABCDEFGHIJKLMNOPQRSTUVWXYZ-search-target-ABCDEFGHIJKLMNOPQRSTUVWXYZ-suffix\n'
```

Search for `search-target`. The result must be found even when the terminal
wrapped that logical line across multiple visual rows.

## 7. Unicode

```bash
printf 'unicode test: A界B界C 日本語 ERROR\n'
```

Search for `界B界` and `日本語`. Highlighting must align with the rendered
wide-character cells.

## 8. Pane isolation

Create two or more split panes.

Pane 1:
```bash
printf 'LEFT-ONLY\n'
```

Pane 2:
```bash
printf 'RIGHT-ONLY\n'
```

`Ctrl+F` in Pane 1 and search `RIGHT-ONLY`: zero matches.
Search in Pane 2: one match.

## 9. Close

`Escape` or the `×` button closes search, removes search highlighting and
returns keyboard focus to the terminal.
