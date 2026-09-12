#!/usr/bin/env bash
# Package one Crashpad build as a sha256-pinned tarball that bugsplat-native's
# cmake/Crashpad.cmake consumes without a GN checkout.
#
#   package.sh <crashpad-root> <out-name> <platform-arch> <dest-dir> [metadata-json]
#
# Layout inside the tarball mirrors the checkout so the CMake glue is identical for a GN build
# and a prebuilt:
#
#   crashpad/
#     client/ util/ snapshot/ minidump/ handler/ tools/ compat/ build/     headers only, plus
#     third_party/mini_chromium/mini_chromium/{base,build}/               handler/win/wer/crashpad_wer.cc
#     third_party/lss/lss/                                                (BugSplatWer.dll compiles it)
#     out/release/obj/**                                                  static libs + tool_support object
#     out/release/gen/**                                                  generated headers
#     out/release/{crashpad_handler,crashpad_wer.dll}                     reference binaries, when built
#     PREBUILT.json                                                       commit, platform, toolchain, gn args
set -euo pipefail

ROOT=$1; OUT=$2; NAME=$3; DEST=$4; META=${5:-'{}'}
COMMIT=$(git -C "$ROOT" rev-parse HEAD)
SHORT=${COMMIT:0:7}
STAGE=$(mktemp -d)
PKG="$STAGE/crashpad"
mkdir -p "$PKG/out/release" "$DEST"

copy_headers() {  # copy_headers <rel-dir> [find-args...]
  local rel=$1; shift
  [ -d "$ROOT/$rel" ] || return 0
  (cd "$ROOT" && find "$rel" -type f \( -name '*.h' "$@" \) -print0) |
    (cd "$ROOT" && xargs -0 -I{} sh -c 'mkdir -p "$0/$(dirname "{}")" && cp "{}" "$0/{}"' "$PKG")
}
for d in client util snapshot minidump handler tools compat build; do copy_headers "$d"; done
copy_headers third_party/mini_chromium/mini_chromium/base
copy_headers third_party/mini_chromium/mini_chromium/build
copy_headers third_party/lss/lss
# Sources that consumers compile themselves (tiny source_sets upstream, not libraries).
if [ -f "$ROOT/handler/win/wer/crashpad_wer.cc" ]; then
  mkdir -p "$PKG/handler/win/wer" && cp "$ROOT/handler/win/wer/crashpad_wer.cc" "$PKG/handler/win/wer/"
fi

# Libraries and the tool_support object, keeping the obj/ tree so paths match a GN build.
(cd "$ROOT/out/$OUT" && find obj -type f \( -name '*.a' -o -name '*.lib' -o -name 'tool_support.tool_support.o' -o -name 'tool_support.tool_support.obj' \) -print0) |
  (cd "$ROOT/out/$OUT" && xargs -0 -I{} sh -c 'mkdir -p "$0/$(dirname "{}")" && cp "{}" "$0/{}"' "$PKG/out/release")
# Test-only archives are dead weight.
find "$PKG/out/release/obj" -type f \( -name '*test*' -o -name 'libgtest*' -o -name 'gmock*' -o -name '*fuzzer*' \) -delete
[ -d "$ROOT/out/$OUT/gen" ] && cp -R "$ROOT/out/$OUT/gen" "$PKG/out/release/gen"
for bin in crashpad_handler crashpad_handler.exe crashpad_wer.dll crashpad_wer.lib; do
  [ -f "$ROOT/out/$OUT/$bin" ] && cp "$ROOT/out/$OUT/$bin" "$PKG/out/release/"
done
cp "$ROOT/out/$OUT/args.gn" "$PKG/out/release/args.gn"
cp "$ROOT/LICENSE" "$PKG/LICENSE"

# Windows hosts may expose a Store stub named python3 that only prints an install hint.
PY=python3; python3 -c pass >/dev/null 2>&1 || PY=python
$PY - "$PKG/PREBUILT.json" "$COMMIT" "$NAME" "$META" "$(cat "$ROOT/out/$OUT/args.gn")" <<'PY'
import json, sys, datetime
path, commit, name, meta, args = sys.argv[1:6]
doc = {"commit": commit, "name": name, "gn_args": args,
       "built": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
doc.update(json.loads(meta))
json.dump(doc, open(path, "w"), indent=2)
PY

TARBALL="crashpad-$SHORT-$NAME.tar.xz"
# Written through stdout: GNU tar reads a "C:" prefix in -f as a remote host on Windows.
tar -C "$STAGE" -cJf - crashpad > "$DEST/$TARBALL"
(cd "$DEST" && sha256sum "$TARBALL" > "$TARBALL.sha256")
echo "packaged $DEST/$TARBALL"
cat "$DEST/$TARBALL.sha256"
du -h "$DEST/$TARBALL"
rm -rf "$STAGE"
