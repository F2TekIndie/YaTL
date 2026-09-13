# Packaging

Run from the repository root:

```sh
./distribution/package.sh
```

This runs the complete build/test loop, stages `make install` without modifying
the host, and creates a `.tar.gz` archive with a SHA-256 checksum. Each run gets
a separate directory under `distribution/artifacts/` containing:

- `rootfs/usr/local/bin/yatl`
- `rootfs/usr/local/bin/yatlctl`
- `rootfs/usr/local/share/applications/org.yatl.YaTL.desktop`
- `rootfs/usr/local/share/applications/org.yatl.YaTL.QuickCapture.desktop`
- `rootfs/usr/local/share/yatl/dms/YaTL/` (DMS widget plugin)
- `rootfs/usr/local/share/yatl/niri/` (opt-in niri fragment and instructions)
- `yatl-linux-<architecture>.tar.gz` and its `.sha256` file

The repository also ships `yatl.spec` for Fedora RPM builds. Build it from a
source tarball with `rpmbuild -ba distribution/yatl.spec` after installing the
listed Fedora `BuildRequires`; the spec installs the same binaries, desktop
entries, DMS widget, and niri fragment under Fedora's standard prefixes.

`QMAKE`, `BUILD_DIR`, and `JOBS` work as in `scripts/verify.sh`. Generated artifacts
are ignored by Git; packaging scripts and documentation are tracked.

The archive is an installation payload, not a standalone Qt bundle or Fedora
RPM. It requires compatible Qt 6 libraries, QML modules, SQLite driver, and
platform plugins on the target. Builds using the personal Qt SDK retain its
runtime path. For Fedora distribution, build against Fedora's Qt packages;
the RPM spec declares the Qt, OpenGL, and libnotify runtime dependencies.
