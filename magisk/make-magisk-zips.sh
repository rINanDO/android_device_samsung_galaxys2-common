#!/usr/bin/env bash
#
# Builds the TWRP-flashable Magisk installer and uninstaller for the i9100:
#
#   ./make-magisk-zips.sh [output-dir]
#
# Output: i9100-magisk-installer.zip and i9100-magisk-uninstaller.zip. Both hold
# only installer/update-binary, so they are built once and work with any Magisk
# release: the installer installs the newest Magisk*.apk in its own folder on
# the phone. Copy the zips next to a Magisk APK and flash the installer in TWRP.
# Needs zip.

set -euo pipefail

here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
out="${1:-$PWD}"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

mkdir -p "$work/zip/META-INF/com/google/android"
cp "$here/installer/update-binary" "$work/zip/META-INF/com/google/android/update-binary"
echo "# Dummy file; update-binary does the work" > "$work/zip/META-INF/com/google/android/updater-script"

mkdir -p "$out"
out="$(cd -- "$out" && pwd)"    # absolute: the zips are made from inside $work
installer="$out/i9100-magisk-installer.zip"
uninstaller="$out/i9100-magisk-uninstaller.zip"
rm -f "$installer" "$uninstaller"

(cd "$work/zip" && zip -q -r "$installer" META-INF)
cp "$installer" "$uninstaller"

echo "$installer"
echo "$uninstaller"
