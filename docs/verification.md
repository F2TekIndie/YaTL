# Verification loop

Run `./scripts/verify.sh` after every iteration. A nonzero exit means the loop
is not ready to hand back. The script performs an incremental build and runs:

| Layer | Checks |
| --- | --- |
| Qt Test | Capture/completion across store restarts; title/ID validation; ordering; populated version-1/version-2 migrations; project/edit/reopen persistence and validation; newer-version refusal; migration rollback; atomic rollback on event failure; model refresh from a second connection |
| CLI integration | Real-process capture/list/complete/history; machine-readable errors; parallel first launch and writes; XDG default path; project/edit/move/reopen loop |
| Qt Quick Test | Real QML screen; empty-input error; keyboard capture; clearing input; completion; Completed/Open navigation; project creation/filtering; edit validation; moving and reopening |

All test databases are temporary. Do not point automated tests at a user's
database. QML tests run offscreen and do not certify Wayland/niri behavior.

## Iteration 1 baseline — September 12, 2026

- Fedora 44, supplied Qt 6.11.2 SDK, qmake, GCC 16.
- `scripts/verify.sh`: passed all 7 domain/model tests, 4 CLI integration tests,
  and the QML workflow test (plus Qt test setup/cleanup checks).
- UI screenshot generated at `build/ui-inbox.png` and visually inspected.
- Desktop entry passed `desktop-file-validate`.
- Staged installation contains exactly the app, CLI, and desktop entry.
- Native Wayland launch stayed running until the smoke-test timeout. Its
  protocol log confirms `xdg_toplevel.set_app_id("org.yatl.YaTL")`.
- The uninstalled desktop entry produces a portal registration warning during
  direct build-tree launch. Installing the entry is still needed for desktop
  registration. niri rules, focus actions, and multi-monitor behavior remain
  manual acceptance work for the desktop integration milestone.
- Fedora CI is configured but has not been run on GitHub from this workspace.
- GCC 16 emits warnings in the supplied Qt headers about incomplete types in
  SFINAE contexts; no project-source compiler warnings remain.

## Manual acceptance on Fedora

Iteration 2 verification (0.2.0): all **11 domain/model tests, 6 CLI tests,
and 2 QML workflows** pass, plus Qt setup/cleanup checks. Coverage includes
upgrading populated version-2 data, retaining event IDs, migration/write
rollback, project filtering, edit validation, moving, reopening, and external
project/task refresh. `build/ui-project.png` and `build/ui-editor.png` were
visually inspected. The updated distribution archive's contents, checksum,
and packaged CLI version were verified. This iteration's UI tests are
offscreen; the native Wayland evidence above is from iteration 1.

1. Start `build/bin/yatl --database /tmp/yatl-manual/tasks.sqlite3`.
2. Submit an empty title. Verify that a useful error appears and capture remains
   usable. Enter a task and press Enter. Verify it appears and the input clears.
3. Run `build/bin/yatlctl --database /tmp/yatl-manual/tasks.sqlite3 add "From CLI"`.
   Verify it appears in the open app within about one second.
4. Complete a task in the app. Verify it disappears from Inbox and appears under
   Completed, and `yatlctl list --filter completed` reports the same data (include
   the same `--database` option).
5. Close and restart the app with the same database. Confirm open and completed
   tasks survive. Run the CLI against the same database for an independent check.
6. Create a project, capture a task there, then edit its title and notes. Submit
   a blank title to verify validation, correct it, and save. Complete and reopen
   the task; then edit its destination to Inbox. Verify both views and restart
   persistence. Test Cancel without saving an edit.
7. On niri, launch with `QT_QPA_PLATFORM=wayland`; inspect the window's app ID as
   `org.yatl.YaTL`. Verify keyboard focus and sizing on the target monitors.

For feedback, record the command, observed result, expected result, and any
terminal/QML diagnostics. Extend the automated coverage when a defect is fixed.
