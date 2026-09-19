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

## Iteration 8 — recurrence and notification scheduling (0.8.0)

- Tasks support daily, weekdays, weekly, and monthly recurrence when at least one
  planning date is present. Completion advances both dates, including weekend
  skipping and Qt calendar month clamping, and transactionally creates one linked
  successor with copied project, list, note, priority, recurrence, and tags.
- A unique source link makes generation idempotent across repeated completion and
  reopen/complete cycles while retaining every completed occurrence in history.
- The main app checks reminders on startup and every minute. Standard
  `notify-send` delivery uses the due date when present and scheduled date
  otherwise; successful sends are recorded and failures remain eligible to retry.
- `yatlctl notify` runs the same scheduler on demand. `add` and `edit` accept
  `--recurrence`, and all task JSON includes recurrence and source-link fields.
- Schema 8 adds recurrence metadata, its uniqueness index, and durable notification
  delivery records. Populated schema-7 migration and rollback fixtures protect all
  existing data.

## Iteration 9 — local settings and desktop preferences (0.9.0)

- Schema 9 stores the default capture project, notification enablement and
  advance window, DMS next-task visibility, and DMS default-project capture choice.
- The main settings dialog edits and validates these values and includes the
  packaged niri include and validation instructions.
- `yatlctl settings` and `settings-set` expose the same preferences for scripts;
  `add --use-default` and the DMS widget honor the configured capture destination.
- Notification scheduling applies the configured advance window and can be
  disabled without deleting delivery history. Migration and rollback fixtures,
  CLI process tests, core tests, and the QML settings workflow cover the loop.

## Iteration 10 — recoverable deletion and interaction polish (1.0.0)

- Schema 10 adds soft-deletion timestamps for tasks and projects. Schema 11
  records which tasks were deleted with a project, so restoring a project does
  not resurrect tasks that were deleted independently.
- Task and project deletion preserve task content, tags, event history, recurrence
  successors, and notification-delivery history. Undo restores the latest deleted
  item; permanent purge detaches surviving recurrence successors before removing
  the deleted records.
- The CLI exposes task and project delete, undelete, and purge commands. The QML
  app exposes task-menu and swipe deletion plus a single, deterministic Undo action.
- Swipe rows reveal Delete on a left swipe and close it on a right swipe. Color
  selection and date keyboard interaction receive additional accessibility polish.
- Task editing is available only from the task action menu. Creating a tag preserves
  the active filter, while visible task-list tag chips select or clear filtering.
- Restart, migration, rollback, tag transaction, recurrence purge, CLI process,
  and QML interaction tests cover the persistence boundary.

## Visual foundation — consolidated plan steps 1–3

- `DmsThemeProvider` reads the active semantic palette from
  `$XDG_CACHE_HOME/DankMaterialShell/dms-colors.json`, watches both the file and
  its directory for atomic replacement, and falls back to the Qt system palette
  without writing to DMS state.
- `AppTheme.qml` exposes the provider's semantic roles and shared spacing,
  sizing, typography, radius, and animation tokens. It contains no fixed
  application colors.
- The reusable DMS-style controls (`AppCard`, `AppButton`, `AppIconButton`,
  `AppTextField`, `AppComboBox`, `AppSwitch`, `AppSegmentedControl`,
  `AppDialog`, `AppMenu`, `AppBadge`, `AppNavigationItem`, `AppSectionHeader`,
  and `AppDateField`) are packaged in the QML module and exercised by the
  component-gallery Qt Quick Test. Core coverage includes light/dark mode,
  malformed and absent cache data, and atomic file replacement.

## Shell and contextual navigation — consolidated plan steps 4–6

- Main and Quick Capture now bind their application palette, surfaces, text,
  borders, and validation states to `AppTheme` and use the shared controls.
- Main has a DMS-style sidebar for Today, Upcoming, Search, Inbox, active
  projects, and New Project, with the existing keyboard and test object names
  preserved for the transition.
- Project, list, tag, archive, and task-state controls stay scoped to the
  selected project page; navigation and search are presented in the page header
  and project context rather than as global organizational controls.

## Capture, task cards, and safe user colors — consolidated plan steps 7–9

- Capture uses a themed, reusable field and button with a visible destination
  hint. `Ctrl+N` switches to the project capture surface before focusing the
  field, so it works from Today, Upcoming, and Search without targeting a
  hidden control.
- Task rows use rounded semantic cards, compact completion and ordering
  controls, metadata hierarchy, and an overflow menu containing Edit, Complete/
  Reopen, and Archive/Restore actions.
- Project and tag colors remain user data and are restricted to small indicators,
  chip borders, and translucent tinted surfaces. Text uses DMS foreground roles
  for readable contrast in both light and dark modes.

## Finalization — editor, polish, export, and Fedora delivery

- The task editor is structured into Task, Organization, Planning, Tags, and
  Notes sections. Date fields provide keyboard-editable ISO dates, clear actions,
  and a popup calendar grid; dialogs use themed surfaces, local validation text,
  and Escape-to-cancel behavior.
- Settings, project, tag, list, and project-creation dialogs use section headers
  and shared controls. Task refreshes compare rows before resetting the model,
  while search remains debounced and tag loading is batched. Sidebar projects are
  filtered to avoid duplicating Inbox and are hosted in a scrollable section.
- `yatlctl export` emits a versioned JSON snapshot of settings, projects, lists,
  tags, and tasks, either to stdout or an atomically-written output file.
- `distribution/yatl.spec` provides Fedora RPM metadata and dependencies. The
  staged tar archive and checksum are produced under `distribution/artifacts/`.
- Button variants, themed combo/spin boxes, reduced-motion environment handling,
  and notification exception boundaries complete the interaction polish.

## Next slices

All planned implementation steps are complete. Remaining acceptance work is
limited to running the Fedora RPM build after installing Fedora's Qt development
packages and completing a real-session desktop smoke test.

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
Export is available through `yatlctl export`, with versioned JSON to stdout or
an atomically-written file. Notifications use the due date (or scheduled date
when no due date exists), with the configured advance window and enablement.
Reopening and archive/restore
are supported; there is no general
edit-undo or project/list deletion feature. Core task edits replace the full
editable record; CLI edits preserve omitted planning values. Concurrent edits
currently use last-writer-wins.
Startup storage failures are reported on stderr; an in-app recovery screen is
future work. Polling is a small initial implementation, not a realtime IPC bus.
