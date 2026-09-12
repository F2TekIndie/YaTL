#!/usr/bin/env bash
set -euo pipefail
distribution_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$distribution_dir/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_dir/build}"
mkdir -p "$build_dir"
build_dir="$(cd -- "$build_dir" && pwd)"

BUILD_DIR="$build_dir" "$repo_dir/scripts/verify.sh"

mkdir -p "$distribution_dir/artifacts"
package_dir="$(mktemp -d "$distribution_dir/artifacts/yatl-XXXXXXXX")"
make -C "$build_dir" install INSTALL_ROOT="$package_dir/rootfs"
archive="yatl-linux-$(uname -m).tar.gz"
tar -czf "$package_dir/$archive" -C "$package_dir/rootfs" .
(
    cd "$package_dir"
    sha256sum "$archive" > "$archive.sha256"
)
echo "Package created: $package_dir/$archive"
