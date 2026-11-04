#!/usr/bin/env bash
# Tests lipo.sh against two tiny single-architecture out dirs (macOS only).
#
#   scripts/farm/test_lipo.sh
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAILED=0

fail() { echo "FAIL: $*" >&2; FAILED=1; }

# archive <out-dir> <arch> <path-under-obj>
archive() {
  local out=$1 arch=$2 lib=$3
  mkdir -p "$out/obj/$(dirname "$lib")"
  echo "int f(void) { return 0; }" > "$WORK/f.c"
  clang -arch "$arch" -c "$WORK/f.c" -o "$WORK/f.o"
  libtool -static -o "$out/obj/$lib" "$WORK/f.o" 2>/dev/null
}

# out_dir <name> <arch> <libs...>
out_dir() {
  local name=$1 arch=$2; shift 2
  local out="$WORK/crashpad/out/$name"
  for lib in "$@"; do archive "$out" "$arch" "$lib"; done
  echo "target_cpu=\"$arch\"" > "$out/args.gn"
}

# Merges every library built for both architectures.
out_dir a arm64 client/libclient.a util/libutil.a
out_dir b x86_64 client/libclient.a util/libutil.a
bash "$HERE/lipo.sh" "$WORK/crashpad" a b u > "$WORK/ok.log" 2>&1 || { cat "$WORK/ok.log"; fail "lipo.sh failed"; }
for lib in client/libclient.a util/libutil.a; do
  archs=$(lipo -archs "$WORK/crashpad/out/u/obj/$lib" 2>/dev/null || true)
  [ "$archs" = "x86_64 arm64" ] || fail "$lib has architectures '$archs'"
done

# Fails on a library that only the second architecture built.
rm -rf "$WORK/crashpad"
out_dir a arm64 client/libclient.a
out_dir b x86_64 client/libclient.a util/libutil.a
if bash "$HERE/lipo.sh" "$WORK/crashpad" a b u > "$WORK/missing.log" 2>&1; then
  fail "lipo.sh succeeded with util/libutil.a built for x86_64 only"
elif ! grep -q "obj/util/libutil.a was built for only one of a and b" "$WORK/missing.log"; then
  cat "$WORK/missing.log"; fail "unexpected error for a one-architecture library"
fi

if [ "$FAILED" -ne 0 ]; then exit 1; fi
echo "lipo.sh tests passed"
