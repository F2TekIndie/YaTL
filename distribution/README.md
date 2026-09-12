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

`QMAKE`, `BUILD_DIR`, and `JOBS` work as in `scripts/verify.sh`. Generated artifacts
are ignored by Git; packaging scripts and documentation are tracked.

The archive is an installation payload, not a standalone Qt bundle or Fedora
RPM. It requires compatible Qt 6 libraries, QML modules, SQLite driver, and
platform plugins on the target. Builds using the personal Qt SDK retain its
runtime path. For Fedora distribution, build against Fedora's Qt packages;
RPM metadata and runtime dependency packaging remain a later step.
