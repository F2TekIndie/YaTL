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

## Iteration 3 — project organization and task ordering (0.3.0)

- Rename projects, assign validated `#RRGGBB` colors, and archive or restore them.
  Archived projects can be shown and reviewed but reject task, list, completion,
  ordering, and project-setting writes until restored.
- Create and rename optional task lists within a project. Capture and edit can
  assign a task to a list or leave it ungrouped; the project view can filter by
  one list, ungrouped tasks, or all tasks.
- Move tasks up and down in the current open/completed and list-filtered view.
  Ordering remains stable across restart and completion/reopen cycles.
- Schema 4 adds project color/archive state, task lists, list membership, ordering,
  indexes, and consistency triggers. Populated schema-3 upgrades retain event IDs,
  notes, task order, and roll back completely if any migration step fails.
- CLI exposes project edit/archive/restore, list create/rename/read, list-aware
  task writes and filters, and ordering. JSON output includes list and order data.
- The QML app exposes project settings, archived-project browsing, list management,
  list-aware editing, list labels, and accessible ordering controls.

## Iteration 4 — planning views and search (0.4.0)

- Store separate scheduled and due local calendar dates plus priority levels from
  0 (none) through 3 (high). Dates use strict `YYYY-MM-DD` values, and a scheduled
  date cannot fall after its due date.
- Today includes open overdue tasks, work due today, and work scheduled by today.
  Upcoming includes open work with either date from tomorrow through 28 days.
  Both views omit tasks in archived projects and order higher priority first.
- Search includes open and completed tasks and matches literal, case-insensitive
  text in task titles, notes, project names, and list names. SQL wildcard
  characters in a query remain literal characters.
- The task editor exposes all planning fields. Cross-project views retain edit and
  completion actions and show the task's project/list context.
- Schema 5 adds indexed planning fields. Populated schema-4 migration tests verify
  retained tasks, lists, history, default values, and full rollback on failure.
- CLI exposes planning options on `add` and `edit`, plus `today`, `upcoming`, and
  `search`. An edit preserves omitted planning values and accepts `none` to clear
  either date.
- Store, real-process CLI, and QML workflows verify validation, restart persistence,
  date boundaries, priority ordering, search fields, archived-project exclusion,
  completion from search, and upgrade rollback.

## Iteration 5 — task archiving and organization ordering (0.5.0)

- Archive open or completed tasks without deleting their content, planning values,
  completion state, or event history. Archived tasks are read-only until restored.
- Open, Completed, Today, and Upcoming omit archived tasks. A project's Archived
  filter retrieves them, while Search includes them when their project is active.
- Projects and task lists now have explicit persistent ordering. New entries append
  to their scope, and bounded Up/Down actions reorder active projects or lists.
- Schema 6 adds task archive state and project/list sort positions, indexes their
  read paths, and expands task history with archive/restore events. Migration keeps
  the previous project/list display order and rebuilds event constraints without
  changing event IDs.
- CLI exposes `archive`, `restore`, `project-move`, and `list-move`; task, project,
  and list JSON includes archive/order state where applicable.
- Store, real-process CLI, and QML workflows verify archive visibility, mutation
  guards, completion retention, search/restore, ordering boundaries, restart
  persistence, non-empty schema-5 upgrades, and migration rollback.

## Iteration 6 — tags and tag-aware retrieval (0.6.0)

- Create and edit global tags with trimmed names, case-insensitive uniqueness,
  a 60-character limit, and normalized validated `#RRGGBB` colors.
- Assign up to 50 distinct tags to a task. Task reads return tags in stable
  case-insensitive name order, and edits record old and new tag IDs in the same
  transactional history event as the other editable task fields.
- Filter any project or Inbox view by one tag. Search matches tag names alongside
  titles, notes, projects, and task lists. Renaming a tag updates display and
  search without rewriting task assignments.
- Schema 7 adds `tags` and the foreign-key-backed many-to-many `task_tags` table.
  Populated schema-6 migration tests verify empty defaults, later assignment,
  retained event IDs, and full rollback when table creation fails.
- CLI exposes `tag-add`, `tag-edit`, and `tags`; `add` and `edit` accept `--tags`,
  while `list --tag` uses the same store filter as the desktop app. JSON task
  output includes full tag ID, name, and color records.
- The QML app exposes the tag filter and tag manager, multi-select assignment in
  the task editor, colored tag labels, and tag-aware search.

## Iteration 7 — DMS and niri desktop integration (0.7.0)

- A local per-database activation socket lets `yatlctl open VIEW`, `focus`, and
  `capture` reuse existing windows. View changes are acknowledged as JSON, and
  niri focus uses its machine-readable window list and stable app IDs.
- A compact title-only quick-capture window writes to Inbox. It has its own
  `org.yatl.YaTL.QuickCapture` app ID and process so niri can float it separately.
- The packaged DMS 1.6 widget shows open and Today counts plus the next actionable
  task. Its popout captures Inbox tasks, completes Today tasks, opens Today, and
  opens quick capture through `yatlctl` JSON commands.
- The optional niri fragment tiles the main window, floats quick capture, and
  binds focus, Today, and capture commands. YaTL documents the include and never
  changes or reloads a user's niri configuration.
- Process tests verify app reuse, view switching, detached launch arguments,
  niri focus dispatch, DMS assets, stable app IDs, and the capture UI. The
  verification loop parses DMS QML and validates the fragment with niri.
- Live Fedora checks loaded the component in DMS 1.6.0 and observed the main app
  tiled, quick capture floating, focus transfer, and placement on both eDP-1 and
  HDMI-A-1.

## Next slices

1. Implement recurrence rules and desktop notification scheduling.
2. Add settings for the default project, notification timing, and desktop options.
3. Add data export, Fedora release packaging, fresh-install/upgrade checks, and
   complete user documentation.

Every slice should extend the shared C++ rules and the same verification script,
including migration fixtures whenever the schema changes. Keep earlier tests
passing. Desktop-specific acceptance remains a real-session smoke test.

## Decisions

- Fedora first; additional distribution coverage belongs in GitHub CI.
- Use `/home/f2tek/SoftwareEngineering/Qt/6.11.2/gcc_64` as the local SDK.
- Use Qt Test/Qt Quick Test; consider Catch2 only when necessary.
- Capture defaults to a title-only Inbox task for the initial loop.
- Planning uses separate scheduled and due local dates. Today includes scheduled
  dates up to today and due dates up to today; Upcoming spans tomorrow through
  28 days and may overlap Today when a task is scheduled now but due later.
- Ask for direction if another required third-party dependency is missing.
- Before recurrence/DMS slices, settle initial recurrence patterns and the DMS
  default destination with the user as needed.

## Deliberate limits

Current summary reports all open tasks across Inbox and active projects and
includes a Today count; the CLI's `today` command returns its detailed rows.
Recurrence, notifications, settings, and export remain unimplemented. DMS quick
capture defaults to Inbox until the settings slice. Reopening and archive/restore
are supported; there is no general
edit-undo or project/list deletion feature. Core task edits replace the full
editable record; CLI edits preserve omitted planning values. Concurrent edits
currently use last-writer-wins.
Startup storage failures are reported on stderr; an in-app recovery screen is
future work. Polling is a small initial implementation, not a realtime IPC bus.
