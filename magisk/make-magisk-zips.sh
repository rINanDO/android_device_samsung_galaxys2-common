#!/usr/bin/env bash
#
# Builds the TWRP-flashable Magisk installer and uninstaller for the i9100 from
# a Magisk release APK:
#
#   ./make-magisk-zips.sh Magisk-v31.1.apk [output-dir]
#
# Output: i9100-magisk-<version>-installer.zip and i9100-magisk-uninstaller.zip.
# Needs zip, unzip and mkbootimg (from the source tree's out/host, or PATH).

set -euo pipefail

here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
apk="${1:?usage: $0 Magisk.apk [output-dir]}"
out="${2:-$PWD}"

[ -f "$apk" ] || { echo "not found: $apk" >&2; exit 1; }
unzip -l "$apk" lib/armeabi-v7a/libmagiskboot.so >/dev/null 2>&1 \
    || { echo "$apk is not a Magisk release APK" >&2; exit 1; }

mkbootimg=$(command -v mkbootimg || true)
if [ -z "$mkbootimg" ]; then
    root="$(cd -- "$here/../../../.." && pwd)"
    for c in "$root/out/host/linux-x86/bin/mkbootimg" "$root/system/tools/mkbootimg/mkbootimg.py"; do
        [ -x "$c" ] && { mkbootimg=$c; break; }
    done
fi
[ -n "$mkbootimg" ] || { echo "mkbootimg not found" >&2; exit 1; }

version=$(unzip -p "$apk" assets/util_functions.sh | sed -n 's/^MAGISK_VER=.\(.*\).$/\1/p')
[ -n "$version" ] || version=unknown

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# A minimal v0 boot image. The installer only uses it as a container: it
# replaces the ramdisk with the device's stock ramdisk before Magisk patches it.
printf 'K' > "$work/kernel"
printf '070701' > "$work/ramdisk"
"$mkbootimg" --kernel "$work/kernel" --ramdisk "$work/ramdisk" --pagesize 2048 \
    --header_version 0 -o "$work/boot-template.img"

mkdir -p "$work/zip/META-INF/com/google/android"
cp "$here/installer/update-binary" "$work/zip/META-INF/com/google/android/update-binary"
echo "# Dummy file; update-binary does the work" > "$work/zip/META-INF/com/google/android/updater-script"

mkdir -p "$out"
out="$(cd -- "$out" && pwd)"    # absolute: the zips are made from inside $work
installer="$out/i9100-magisk-v$version-installer.zip"
uninstaller="$out/i9100-magisk-uninstaller.zip"
rm -f "$installer" "$uninstaller"

(cd "$work/zip" && zip -q -r "$uninstaller" META-INF)
cp "$apk" "$work/zip/Magisk.apk"
cp "$work/boot-template.img" "$work/zip/boot-template.img"
(cd "$work/zip" && zip -q -r "$installer" META-INF Magisk.apk boot-template.img)

echo "$installer"
echo "$uninstaller"
