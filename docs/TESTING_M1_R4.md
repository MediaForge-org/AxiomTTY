# M1 R4 manual test

## Copy / interrupt

1. Run `echo "copy this text"`.
2. Drag-select `copy this text` with the left mouse button.
3. The bottom-right hint must say `CTRL+C COPY`.
4. Press **Ctrl+C**. The hint briefly changes to `COPIED` and Bash must not print `^C`.
5. Press **Ctrl+V**. The copied text should be pasted at the prompt.
6. With no selection, run `sleep 30` and press **Ctrl+C**. The process must stop.

`Ctrl+Shift+C` and `Ctrl+Shift+V` remain aliases.

## Scrollback selection

Run `seq 1 300`, scroll upward, select old output, press Ctrl+C, return to the bottom and paste with Ctrl+V.

## Input regression

Check Enter, Backspace, Tab, arrows, Home/End, Delete, Ctrl+R, Ctrl+Z and normal text entry.
