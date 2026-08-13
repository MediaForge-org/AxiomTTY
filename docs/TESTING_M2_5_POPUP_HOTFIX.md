# M2.5 close-dialog hotfix

1. Start `sleep 60` in a pane.
2. Press Ctrl+Shift+X repeatedly and test Cancel with the mouse at least 10 times.
3. Open it again and test Close anyway with the mouse at least 10 times across fresh panes.
4. Test Tab to switch button focus, Enter to activate the focused button, and Escape to cancel.
5. Repeat for Ctrl+Shift+W on a busy tab and the window close button while a busy process is running.
6. Cancel must never terminate the session. Close anyway must close exactly the requested pane/tab/application.
