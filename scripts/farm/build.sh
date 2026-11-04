#!/usr/bin/env bash
# Build Crashpad at a pinned commit with GN + depot_tools (POSIX hosts: Linux, macOS,
# Android and iOS/tvOS cross builds).
#
#   build.sh <work-dir> <commit> <out-name> "<args.gn contents>" <ninja targets...>
#
# The checkout lands in <work-dir>/crashpad and the build in <work-dir>/crashpad/out/<out-name>.
# Re-runnable: an existing checkout is reused and re-synced at the commit.
set -euo pipefail

WORK=$1; COMMIT=$2; OUT=$3; ARGS=$4; shift 4
[ $# -gt 0 ] || { echo "no ninja targets given" >&2; exit 2; }

mkdir -p "$WORK"
if [ ! -d "$WORK/depot_tools" ]; then
  git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git "$WORK/depot_tools"
fi
export PATH="$WORK/depot_tools:$PATH"
gclient --version >/dev/null   # bootstraps depot_tools' own python/ninja/gn

if [ ! -d "$WORK/crashpad" ]; then
  (cd "$WORK" && fetch --no-history crashpad)
fi
cd "$WORK/crashpad"
git fetch --depth 1 origin "$COMMIT"
git checkout -q "$COMMIT"
gclient sync

# mini_chromium's Android toolchain sets tool_prefix for 32-bit ARM and never reads it, and GN
# treats an unused assignment as an error; 32-bit ARM cannot be configured without this one-line
# not_needed(). Build-config only, no Crashpad source is touched; package.sh records it.
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
