#!/usr/bin/env bash
# Builds Crashpad at <commit> into <work-dir>/crashpad/out/<out-name> with depot_tools at
# $DEPOT_TOOLS_COMMIT. Re-running reuses the checkout.
#
#   build.sh <work-dir> <commit> <out-name> "<args.gn contents>" <ninja targets...>
set -euo pipefail

WORK=$1; COMMIT=$2; OUT=$3; ARGS=$4; shift 4
[ $# -gt 0 ] || { echo "no ninja targets given" >&2; exit 2; }
: "${DEPOT_TOOLS_COMMIT:?set DEPOT_TOOLS_COMMIT to the depot_tools commit to build with}"

mkdir -p "$WORK"
if [ ! -d "$WORK/depot_tools" ]; then
  git init -q "$WORK/depot_tools"
  git -C "$WORK/depot_tools" remote add origin https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi
git -C "$WORK/depot_tools" fetch -q --depth 1 origin "$DEPOT_TOOLS_COMMIT"
git -C "$WORK/depot_tools" checkout -q FETCH_HEAD
# Otherwise every gclient call moves depot_tools to origin/main.
export DEPOT_TOOLS_UPDATE=0
export PATH="$WORK/depot_tools:$PATH"
bash "$WORK/depot_tools/ensure_bootstrap"

if [ ! -d "$WORK/crashpad" ]; then
  (cd "$WORK" && fetch --no-history crashpad)
fi
cd "$WORK/crashpad"
git fetch --depth 1 origin "$COMMIT"
git checkout -q "$COMMIT"
gclient sync

# mini_chromium's 32-bit ARM Android toolchain sets tool_prefix without reading it, which GN
# rejects. package.sh records the patch in PREBUILT.json.
if [[ "$ARGS" == *'target_os="android"'* && "$ARGS" == *'target_cpu="arm"'* ]]; then
  python3 - third_party/mini_chromium/mini_chromium/build/config/BUILD.gn <<'PY'
import io, sys
p = sys.argv[1]; s = io.open(p, encoding="utf-8").read()
anchor = '      tool_prefix = "arm-linux-androideabi"' + chr(10)
fix = anchor + '      not_needed([ "tool_prefix" ])' + chr(10)
if anchor in s and fix not in s:
    io.open(p, "w", encoding="utf-8").write(s.replace(anchor, fix)); print("patched", p)
PY
fi

mkdir -p "out/$OUT"
printf '%s\n' "$ARGS" > "out/$OUT/args.gn"
echo "--- out/$OUT/args.gn"; cat "out/$OUT/args.gn"; echo "---"
gn gen "out/$OUT"
ninja -C "out/$OUT" "$@"
