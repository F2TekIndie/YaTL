# Verification loop

Run `./scripts/verify.sh` after every iteration. A nonzero exit means the loop
is not ready to hand back. The script performs an incremental build and runs:

| Layer | Checks |
| --- | --- |
| Qt Test | Capture/completion/archive across store restarts; title/ID/planning/tag validation; Today/Upcoming boundaries; literal and tag-aware search; populated version-1 through version-6 migrations; project/list/task archive and ordering persistence; mutation guards; newer-version refusal; migration and write rollback; external connection refresh |
| CLI integration | Real-process capture/list/complete/history; machine-readable errors; parallel first launch and writes; XDG default path; project/task archive and restore, project/list/task ordering, editing, reopening, planning views, tags, and search |
| Qt Quick Test | Real QML screen; capture and planning validation; completion/reopening; project/task archive and restore; project/list/tag create, edit, filter, assignment, and ordering controls; task editing; Today/Upcoming/search navigation and contextual results |
| Desktop process | App reuse and view switching; niri focus command; detached main/capture launch; stable desktop IDs; DMS manifest and command boundary |
| Desktop syntax | DMS QML parsing with the selected Qt toolchain and `niri validate` on the packaged fragment |

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

Iteration 3 manual checks extend step 6 below with project colors, list creation
and rename, All/ungrouped/list filters, Up/Down ordering, archive browsing, and
restore. Archived projects must disable every write control while retaining
their tasks and lists.

Iteration 3 automated verification (0.3.0): all **14 domain/model tests, 8 CLI
integration tests, and 3 QML workflows** pass, plus Qt setup/cleanup checks.
The project-management workflow screenshot at `build/ui-managed-project.png`
was visually inspected. The packaged CLI reports 0.3.0, the desktop entry is
valid, and the generated archive contains only the app, CLI, and desktop entry;
its SHA-256 checksum passes.

Iteration 4 automated verification (0.4.0): all **17 domain/model tests, 10 CLI
integration tests, and 4 QML workflows** pass, plus Qt setup/cleanup checks.
Coverage includes strict planning validation, Today and 28-day Upcoming boundaries,
priority ordering, literal search across all supported text fields, archived-project
exclusion, edit preservation/clearing, completion from a cross-project view, schema-4
upgrade defaults, and migration rollback. `build/ui-planning.png` was visually
inspected. The packaged CLI reports 0.4.0, the staged desktop entry validates,
the archive contains exactly the app, CLI, and desktop entry, and its SHA-256
checksum passes.

Iteration 5 automated verification (0.5.0): all **20 domain/model tests, 11 CLI
integration tests, and 5 QML workflows** pass, plus Qt setup/cleanup checks.
Coverage includes open and completed task archiving, read-only archive guards,
search and restore, active-planning exclusion, project/list ordering boundaries,
restart persistence, history retention, populated schema-5 upgrade ordering, and
migration rollback. `build/ui-archive-order.png` was visually inspected. The
packaged CLI reports 0.5.0, the staged desktop entry validates, the archive
contains exactly the app, CLI, and desktop entry, and its SHA-256 checksum passes.

Iteration 6 automated verification (0.6.0): all **22 domain/model tests, 12 CLI
integration tests, and 6 QML workflows** pass, plus Qt setup/cleanup checks.
Coverage includes tag validation and case-insensitive uniqueness, multiple task
assignments, edit-history snapshots, project filtering, tag-aware search, rename
propagation, clearing, restart persistence, populated schema-6 upgrade defaults,
and migration rollback. `build/ui-tags.png` was visually inspected. The packaged
CLI reports 0.6.0, the staged desktop entry validates, the archive contains exactly
the app, CLI, and desktop entry, and its SHA-256 checksum passes.

Iteration 7 automated verification (0.7.0): all **22 domain/model tests, 12 CLI
integration tests, 4 desktop-process/contract tests, and 7 QML workflows** pass,
plus Qt setup/cleanup checks. DMS QML parsing and `niri validate` also pass. A
live Fedora 44 session with DMS 1.6.0 and niri 26.04 loaded the YaTL plugin,
reused one main process while changing Today to Upcoming, focused its tiled
`org.yatl.YaTL` window, launched `org.yatl.YaTL.QuickCapture`, moved capture to
floating mode, and verified it on the second connected output. The niri rule
fragment itself remains opt-in as required, so the smoke test did not edit or
reload the user's active compositor configuration.

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
7. Edit a task with today's scheduled date, tomorrow's due date, and high priority.
   Confirm it appears in both Today and Upcoming with its project/list context.
   Search for its title, note, project, and list; complete it from Search and confirm
   the completed result remains searchable. Verify a scheduled date after the due
   date is rejected without closing the editor.
8. Archive an open task and a completed task. Confirm both leave active views,
   appear in the project's Archived filter, remain discoverable in Search, and
   restore to their previous completion state. Reorder three projects and three
   task lists, restart YaTL, and confirm the display order survives.
9. Create two tags with distinct colors, assign both to a task in Edit, and filter
   the project by each tag. Rename one tag and confirm its task label and a search
   for the new name update. Clear all tags and confirm the tag filters no longer
   return the task.
10. On niri, include the packaged `yatl.kdl`, validate/reload your configuration,
    and invoke each documented keybind. Confirm the main window is tiled, capture
    is floating, repeated commands reuse each surface, and both work on each output.
11. Copy the packaged DMS plugin to the user plugin directory, scan and enable it,
    and add YaTL to DankBar. Confirm the pill summary updates, the popout captures
    and completes tasks, and Open Today and Quick capture reuse their windows.

For feedback, record the command, observed result, expected result, and any
terminal/QML diagnostics. Extend the automated coverage when a defect is fixed.
