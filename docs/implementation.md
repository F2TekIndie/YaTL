# Implementation status

Source of scope: `YaTL_Project_Plan.docx` (September 12, 2026). The document
explicitly specifies **qmake**, not CMake.

## Iteration 1 — complete task loop

- qmake root project builds a shared static C++ core, QML app, CLI, and tests.
- Title-only capture defaults to Inbox. Whitespace is trimmed; empty titles and
  titles over 500 UTF-16 code units are rejected with visible feedback.
- Inbox and Completed views show retained tasks; completion is idempotent.
- Qt SQL SQLite storage uses the XDG data location, prepared statements, foreign
  keys, a five-second busy timeout, and atomic task/event transactions.
- Transactional migrations upgrade version 0 → 1 (tasks) → 2 (events and indexes).
  Version 1 tasks are retained and their history is backfilled. Newer schemas
  are refused. Migration failures roll back schema and version changes together.
- The app polls SQLite's connection-local `data_version` once per second to
  refresh after external commits. Successful in-app changes refresh immediately.
- `yatlctl add`, `list`, `complete`, and `summary` provide a JSON boundary for
  repeatable process tests and future desktop integration.
- Stable desktop ID and desktop entry establish the niri integration boundary.

This spans the foundation and a narrow part of core tasks, bringing basic CLI
actions forward so each iteration can be verified through real processes.
It does not claim completion of all milestone 2 or desktop integration features.

## Iteration 2 — projects, editing, and reopening (0.2.0)

- Create named projects and switch between project task views and Inbox.
- Capture into the selected destination; edit titles and plain-text notes; move
  tasks to another project or back to Inbox.
- Reopen completed work without deleting its completion history.
- Schema 3 adds projects, task project/notes fields, and edit/reopen events with
  previous/new values. Version-1 and version-2 upgrade fixtures retain data and
  event IDs. Edits and their events commit or roll back together.
- CLI exposes `project-add`, `projects`, `edit`, and `reopen`, plus project filters.
  CLI `edit` explicitly requires the complete replacement details.
- The app refreshes project choices and scoped tasks after external writes.
- Expanded domain, real-process CLI, and QML tests cover the full project loop.
- Updated distribution payload built using the existing packaging script.

## Next slices

1. Project rename/archive, task ordering, and optional task lists.
2. Separate scheduled/due dates, Today, and search; then Upcoming and priority.
3. Tags, recurrence, and desktop notification scheduling.
4. DMS widget/popout and CLI open/focus actions; verify against installed DMS APIs.
5. niri tiled/floating, app-ID and multi-monitor verification, then export and
   packaging/release checks.

Every slice should extend the shared C++ rules and the same verification script,
including migration fixtures whenever the schema changes. Keep earlier tests
passing. Desktop-specific acceptance remains a real-session smoke test.

## Decisions

- Fedora first; additional distribution coverage belongs in GitHub CI.
- Use `/home/f2tek/SoftwareEngineering/Qt/6.11.2/gcc_64` as the local SDK.
- Use Qt Test/Qt Quick Test; consider Catch2 only when necessary.
- Capture defaults to a title-only Inbox task for the initial loop.
- Ask for direction if another required third-party dependency is missing.
- Before planning/recurrence/DMS slices, settle date semantics, initial recurrence
  patterns, and DMS default destination with the user as needed.

## Deliberate limits

Current summary reports all open tasks across Inbox and projects; it is not a
Today summary. Tasks are newest-first. Project rename/archive, task ordering,
optional task lists, dates, search, recurrence, notifications, DMS, and
single-instance/focus behavior remain unimplemented. Reopening is supported;
there is no general edit-undo or deletion feature. Task edits replace the full
set of editable details; concurrent edits currently use last-writer-wins.
Startup storage failures are reported on stderr; an in-app recovery screen is
future work. Polling is a small initial implementation, not a realtime IPC bus.
