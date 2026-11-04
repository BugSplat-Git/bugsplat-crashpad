#!/usr/bin/env bash
# Packages one Crashpad build as crashpad-<commit7>-<platform-arch>.tar.xz plus its .sha256. The
# tarball mirrors a Crashpad checkout (headers, out/release/{obj,gen}, PREBUILT.json, licenses) so
# bugsplat-native builds against a prebuilt and a GN build the same way.
#
#   package.sh <crashpad-root> <out-name> <platform-arch> <dest-dir> [metadata-json]
set -euo pipefail

ROOT=$1; OUT=$2; NAME=$3; DEST=$4; META=${5:-'{}'}
COMMIT=$(git -C "$ROOT" rev-parse HEAD)
SHORT=${COMMIT:0:7}
STAGE=$(mktemp -d)
PKG="$STAGE/crashpad"
mkdir -p "$PKG/out/release" "$DEST"

# xargs would hit BSD xargs' line-length limit on this many headers.
copy_tree() {  # copy_tree <src-root> <dest-root> <find expression...>
  local src=$1 dst=$2; shift 2
  mkdir -p "$dst"
  (cd "$src" && find "$@" -print0 | tar -c --null -T - -f -) | (cd "$dst" && tar -x -f -)
}
copy_headers() {  # copy_headers <rel-dir>
  [ -d "$ROOT/$1" ] || return 0
  copy_tree "$ROOT" "$PKG" "$1" -type f -name '*.h'
}
for d in client util snapshot minidump handler tools compat build; do copy_headers "$d"; done
copy_headers third_party/mini_chromium/mini_chromium/base
copy_headers third_party/mini_chromium/mini_chromium/build
copy_headers third_party/lss/lss
# BugSplatWer.dll compiles this itself.
if [ -f "$ROOT/handler/win/wer/crashpad_wer.cc" ]; then
  mkdir -p "$PKG/handler/win/wer" && cp "$ROOT/handler/win/wer/crashpad_wer.cc" "$PKG/handler/win/wer/"
fi

copy_tree "$ROOT/out/$OUT" "$PKG/out/release" obj -type f \( -name '*.a' -o -name '*.lib' -o -name 'tool_support.tool_support.o' -o -name 'tool_support.tool_support.obj' \)
find "$PKG/out/release/obj" -type f \( -name '*test*' -o -name 'libgtest*' -o -name 'gmock*' -o -name '*fuzzer*' \) -delete
[ -d "$ROOT/out/$OUT/gen" ] && cp -R "$ROOT/out/$OUT/gen" "$PKG/out/release/gen"
for bin in crashpad_handler crashpad_handler.exe crashpad_wer.dll crashpad_wer.lib; do
  [ -f "$ROOT/out/$OUT/$bin" ] && cp "$ROOT/out/$OUT/$bin" "$PKG/out/release/"
done
cp "$ROOT/out/$OUT/args.gn" "$PKG/out/release/args.gn"
cp "$ROOT/LICENSE" "$PKG/LICENSE"

# Every third-party component shipped as headers or libraries ships with the license file its
# README.crashpad names.
THIRD_PARTY=$(for d in "$PKG/third_party" "$PKG/out/release/obj/third_party"; do
  if [ -d "$d" ]; then ls "$d"; fi
done | sort -u)
for name in $THIRD_PARTY; do
  readme="$ROOT/third_party/$name/README.crashpad"
  license=; file=
  if [ -f "$readme" ]; then
    license=$(tr -d '\r' < "$readme" | sed -n 's/^License: *//p' | head -1)
    file=$(tr -d '\r' < "$readme" | sed -n 's/^License File: *//p' | head -1)
  fi
  if [ -z "$license" ] || [ -z "$file" ] || [ ! -f "$ROOT/third_party/$name/$file" ]; then
    echo "package.sh: third_party/$name is shipped but has no license file named in README.crashpad" >&2
    exit 1
  fi
  mkdir -p "$PKG/third_party_licenses/$name"
  cp "$ROOT/third_party/$name/$file" "$PKG/third_party_licenses/$name/"
  echo "$name: $license ($file)" >> "$PKG/third_party_licenses/README"
done

# Windows hosts may expose a Store stub named python3 that only prints an install hint.
PY=python3; python3 -c pass >/dev/null 2>&1 || PY=python
PATCHES=$(git -C "$ROOT/third_party/mini_chromium/mini_chromium" diff --name-only 2>/dev/null | tr '
' ' ')
$PY - "$PKG/PREBUILT.json" "$COMMIT" "$NAME" "$META" "$(cat "$ROOT/out/$OUT/args.gn")" "$PATCHES" <<'PY'
import json, sys, datetime
path, commit, name, meta, args, patches = sys.argv[1:7]
doc = {"commit": commit, "name": name, "gn_args": args,
       "built": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
       "buildConfigPatches": ["third_party/mini_chromium/mini_chromium/" + p for p in patches.split()]}
doc.update(json.loads(meta))
json.dump(doc, open(path, "w"), indent=2)
PY

TARBALL="crashpad-$SHORT-$NAME.tar.xz"
# Written through stdout: GNU tar reads a "C:" prefix in -f as a remote host on Windows.
tar -C "$STAGE" -cJf - crashpad > "$DEST/$TARBALL"
if command -v sha256sum >/dev/null 2>&1; then SHA256=(sha256sum); else SHA256=(shasum -a 256); fi
(cd "$DEST" && "${SHA256[@]}" "$TARBALL" > "$TARBALL.sha256")
echo "packaged $DEST/$TARBALL"
cat "$DEST/$TARBALL.sha256"
du -h "$DEST/$TARBALL"
rm -rf "$STAGE"
