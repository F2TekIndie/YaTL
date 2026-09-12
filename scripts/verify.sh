#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_dir/build}"
qt_qmake="${QMAKE:-}"
if [[ -z "$qt_qmake" ]]; then
    if [[ -x /home/f2tek/SoftwareEngineering/Qt/6.11.2/gcc_64/bin/qmake ]]; then
        qt_qmake=/home/f2tek/SoftwareEngineering/Qt/6.11.2/gcc_64/bin/qmake
    elif command -v qmake6 >/dev/null 2>&1; then
        qt_qmake="$(command -v qmake6)"
    else
        echo 'Qt 6 qmake is missing. Set QMAKE=/path/to/Qt/6.x/gcc_64/bin/qmake.' >&2
        exit 1
    fi
fi
mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
cd "$build_dir"
"$qt_qmake" "$repo_dir/YaTL.pro"
make -j"${JOBS:-4}"
"$build_dir/bin/tst_core"
python3 "$repo_dir/tests/cli/test_cli.py" "$build_dir/bin/yatlctl"
python3 "$repo_dir/tests/desktop/test_desktop.py" "$build_dir/bin/yatl" "$build_dir/bin/yatlctl" "$repo_dir"
qt_host_bins="$("$qt_qmake" -query QT_HOST_BINS)"
for qml_file in "$repo_dir"/integrations/dms/YaTL/*.qml; do
    "$qt_host_bins/qmlformat" "$qml_file" >/dev/null
done
niri validate --config "$repo_dir/integrations/niri/yatl.kdl"
QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME=generic QT_QUICK_CONTROLS_STYLE=Fusion \
    QT_QUICK_BACKEND=software QSG_RHI_BACKEND=software \
    "$build_dir/bin/tst_ui" -input "$repo_dir/tests/ui"
