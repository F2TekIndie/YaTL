# YaTL

A private, Linux-only project and todo manager built with C++17, Qt 6 Quick/QML,
qmake, and SQLite. Fedora is the first supported target.

The implementation covers **project → capture → edit → complete → reopen →
restart**, alongside the original Inbox workflow. The app and `yatlctl` share
validation, transactional writes, and the same local database. Planning and
DMS integration are subsequent iterations; see [implementation status](docs/implementation.md).

## Build and verify

Required: a C++17 compiler, Make, Python 3 (standard library only), Qt 6 Core,
SQL with the SQLite driver, Quick, Quick Controls 2, Qt Test, Qt Quick Test, qmake,
and the system OpenGL development library.

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
sudo dnf install gcc-c++ make python3 qt6-qtbase-devel qt6-qtdeclarative-devel libglvnd-devel
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
./build/bin/yatlctl summary
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
notes, and destination. Invalid edits keep the dialog open; Cancel discards
unsaved changes. **Reopen** returns completed work to the Open view.

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
```

Substitute the returned project and task IDs. `edit` replaces the title, note,
and destination together, so `--project` and `--note` are required; pass
`--note ""` to clear notes. `list` defaults to all projects for compatibility;
`--project inbox` restricts it to unassigned tasks. `summary` counts open tasks
across all projects. Project names are trimmed, limited to 120 characters, and
unique under SQLite's ASCII case-insensitive comparison. Notes allow up to
100,000 UTF-16 code units. Migration to schema 3 preserves existing tasks and
events, and leaves old tasks in Inbox.

## Desktop and installation

Run `./distribution/package.sh` to verify, stage, and archive the installation
payload in `distribution/artifacts/`. See [packaging](distribution/README.md)
for contents and runtime requirements.

`QT_QPA_PLATFORM=wayland ./build/bin/yatl` explicitly selects Wayland. The stable
desktop/application ID is `org.yatl.YaTL`. A desktop entry is provided in
`integrations/niri/`; no compositor configuration is modified.

After building, `make -C build install INSTALL_ROOT=/tmp/yatl-package` stages
the app, CLI, and desktop entry beneath `/tmp/yatl-package/usr/local/`.
This is a staging layout, not yet a redistributable bundle: a build against a
personal Qt SDK uses that SDK's runtime location. Build against Fedora's Qt
packages for system packaging.

See [verification](docs/verification.md) for the repeatable manual feedback loop.
