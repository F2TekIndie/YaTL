# YaTL

A private, Linux-only project and todo manager built with C++17, Qt 6 Quick/QML,
qmake, and SQLite. Fedora is the first supported target.

The implementation covers **capture → organize/order/tag → schedule → review Today/Upcoming
→ search → complete → archive/restore → restart**, alongside project, list, and tag management.
The app and `yatlctl` share validation, transactional writes, and the same local
database. DMS and niri integrations use the same CLI and activation path.
Recurrence, notifications, settings, and export follow in later iterations; see
[implementation status](docs/implementation.md).

## Build and verify

Required: a C++17 compiler, Make, Python 3 (standard library only), Qt 6 Core,
SQL with the SQLite driver, Network, Quick, Quick Controls 2, Qt Test, Qt Quick
Test, qmake, niri (for fragment validation), and the system OpenGL development library.

Use the supplied Qt SDK:

```sh
QMAKE=/home/f2tek/SoftwareEngineering/Qt/6.11.2/gcc_64/bin/qmake ./scripts/verify.sh
```

The script builds in `build/` and runs domain/repository, CLI-process, and QML
interaction tests. Tests use temporary databases and the QML tests render
offscreen. `QMAKE`, `BUILD_DIR`, and `JOBS` can override the defaults.
The same script is configured in `.github/workflows/verify.yml` for Fedora 44.

For a Fedora system toolchain, install dependencies yourself:

```sh
sudo dnf install gcc-c++ make python3 qt6-qtbase-devel qt6-qtdeclarative-devel libglvnd-devel niri
QMAKE=/usr/bin/qmake6 ./scripts/verify.sh
```

With the supplied Qt SDK, only missing system dependencies need installation.
Qt Test and Qt Quick Test cover the current scope; Catch2 is the fallback if
future test needs exceed their capabilities.

## Run

```sh
./build/bin/yatl
./build/bin/yatlctl add "Verify the first loop"
./build/bin/yatlctl list
./build/bin/yatlctl complete 1
./build/bin/yatlctl list --filter completed
./build/bin/yatlctl add "Plan the release" --scheduled 2026-09-12 --due 2026-09-13 --priority 3
./build/bin/yatlctl today
./build/bin/yatlctl upcoming
./build/bin/yatlctl tag-add "Work" --color "#59675c"
./build/bin/yatlctl add "Tagged task" --tags 1
./build/bin/yatlctl list --tag 1
./build/bin/yatlctl search "release"
./build/bin/yatlctl archive 1
./build/bin/yatlctl restore 1
./build/bin/yatlctl summary
./build/bin/yatlctl open today
./build/bin/yatlctl focus
./build/bin/yatlctl capture
```

Use the actual ID returned by `add`. Successful CLI commands emit JSON to stdout.
Failures emit JSON to stderr and return 1 for storage/validation failures or 2
for command syntax errors. `--help` and `--version` emit human-readable text.
Titles are single quoted shell arguments; for a title starting with a dash,
use `yatlctl add -- "--literal title"`.

Both executables use `$XDG_DATA_HOME/yatl/yatl.sqlite3`, falling back to
`~/.local/share/yatl/yatl.sqlite3`. To experiment without touching your tasks:

```sh
./build/bin/yatl --database /tmp/yatl-demo/tasks.sqlite3
./build/bin/yatlctl --database /tmp/yatl-demo/tasks.sqlite3 add "Try capture"
```

External CLI changes refresh in the app within approximately one second.
Ctrl+N focuses capture; Enter adds the task; F5 refreshes. Completed tasks remain
available in the Completed view, including after restart.

Choose Inbox or a project in the selector. **New project** creates and selects a
project; capture adds tasks to that destination. **Edit** changes the title,
notes, destination, separate scheduled and due dates, priority, and multiple tags. Invalid edits
keep the dialog open; Cancel discards unsaved changes. **Today** shows open work
that is overdue, due today, or scheduled by today. **Upcoming** shows open work
with either date from tomorrow through the next 28 days. Search finds open and
completed tasks by title, note, project, list, or tag name. The tag selector filters
the current project or Inbox; **New tag** and **Edit tag** manage the global tag list.
**Reopen** returns completed
work to the Open view. **Archive** removes a task from active and planning views;
the project's Archived filter or Search can retrieve and restore it. Arrow controls
beside project and list selectors set their persistent display order.

The equivalent CLI workflow is:

```sh
./build/bin/yatlctl project-add "Release"
./build/bin/yatlctl projects
./build/bin/yatlctl add "Draft release notes" --project 1
./build/bin/yatlctl list --project 1
./build/bin/yatlctl edit 1 "Publish release notes" --project 1 --note "Review first"
./build/bin/yatlctl complete 1
./build/bin/yatlctl reopen 1
./build/bin/yatlctl edit 1 "Publish release notes" --project inbox --note "Review first"
./build/bin/yatlctl project-edit 1 "Release 1.0" --color "#663399"
./build/bin/yatlctl list-add 1 "Review"
./build/bin/yatlctl add "Check package" --project 1 --list 1
./build/bin/yatlctl move 2 up --list 1
./build/bin/yatlctl project-archive 1
./build/bin/yatlctl projects --archived
./build/bin/yatlctl project-restore 1
./build/bin/yatlctl project-move 1 up
./build/bin/yatlctl edit 2 "Check package" --project 1 --list 1 --note "" --scheduled 2026-09-12 --due 2026-09-13 --priority 3
./build/bin/yatlctl list-move 1 up
./build/bin/yatlctl tag-add "Work" --color "#59675c"
./build/bin/yatlctl tag-add "Urgent" --color "#aa2727"
./build/bin/yatlctl tags
./build/bin/yatlctl edit 2 "Check package" --project 1 --list 1 --note "" --tags 1,2
./build/bin/yatlctl list --project 1 --tag 1
./build/bin/yatlctl tag-edit 1 "Office" --color "#334455"
./build/bin/yatlctl today
./build/bin/yatlctl upcoming
./build/bin/yatlctl search "package"
./build/bin/yatlctl archive 2
./build/bin/yatlctl list --project 1 --filter archived
./build/bin/yatlctl restore 2
```

Substitute the returned project, list, and task IDs. `edit` replaces the title,
note, and destination together, so `--project` and `--note` are required; pass
`--note ""` to clear notes. Planning and tag options on `edit` preserve their
existing values when omitted; pass `--scheduled none`, `--due none`,
`--priority 0`, or `--tags none` to clear them. Dates are strict local calendar
dates in `YYYY-MM-DD` form, and a scheduled date cannot be later than its due
date. Priorities range from 0 (none)
through 3 (high). `list` defaults to all projects for compatibility;
`--project inbox` restricts it to unassigned tasks. `summary` counts open tasks
across all projects. Project names are trimmed, limited to 120 characters, and
unique under SQLite's ASCII case-insensitive comparison. Notes allow up to
100,000 UTF-16 code units. Projects have a validated `#RRGGBB` color and may be
archived; archived projects remain browseable and become read-only until restored.
Archived tasks retain their completion state and history, remain searchable while
their project is active, and become read-only until restored. Optional task lists
belong to one project. Up/Down ordering persists for projects, task lists, and tasks
within filtered project views. Tags are global, case-insensitively unique, limited
to 60 characters, and use validated `#RRGGBB` colors. A task accepts at most 50
distinct tags. Migration to schema 7 preserves existing projects, lists, tasks,
events, planning values, and visible ordering; existing tasks begin with no tags.

## Desktop and installation

Run `./distribution/package.sh` to verify, stage, and archive the installation
payload in `distribution/artifacts/`. See [packaging](distribution/README.md)
for contents and runtime requirements.

`QT_QPA_PLATFORM=wayland ./build/bin/yatl` explicitly selects Wayland. The main
and capture app IDs are `org.yatl.YaTL` and `org.yatl.YaTL.QuickCapture`.
`yatlctl open VIEW` changes the active view in an existing process or launches
one, `focus` raises it through niri, and `capture` reuses a compact Inbox window.

The package contains an optional niri fragment with tiled/floating rules and
three keybinds. See [niri integration](integrations/niri/README.md). It also
contains the YaTL DankMaterialShell 1.6 widget and popout; see
[DMS integration](integrations/dms/README.md). Neither integration rewrites the
user's compositor or shell configuration.

After building, `make -C build install INSTALL_ROOT=/tmp/yatl-package` stages
the app, CLI, desktop entries, DMS plugin, and niri fragment beneath
`/tmp/yatl-package/usr/local/`.
This is a staging layout, not yet a redistributable bundle: a build against a
personal Qt SDK uses that SDK's runtime location. Build against Fedora's Qt
packages for system packaging.

See [verification](docs/verification.md) for the repeatable manual feedback loop.
