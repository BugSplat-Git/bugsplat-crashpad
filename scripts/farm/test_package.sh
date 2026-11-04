#!/usr/bin/env bash
# Tests package.sh against a minimal fake Crashpad checkout (no GN build needed).
#
#   scripts/farm/test_package.sh
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAILED=0

fail() { echo "FAIL: $*" >&2; FAILED=1; }

# fake_checkout <root>: Crashpad's LICENSE, a header, mini_chromium headers (copied by package.sh)
# and a static library built from third_party/zlib, each with a README.crashpad.
fake_checkout() {
  local root=$1
  mkdir -p "$root/client" "$root/out/test/obj/client" "$root/out/test/obj/third_party/zlib" \
    "$root/third_party/mini_chromium/mini_chromium/base" "$root/third_party/zlib/zlib"
  echo "Apache License" > "$root/LICENSE"
  echo "// client" > "$root/client/client.h"
  echo "// base" > "$root/third_party/mini_chromium/mini_chromium/base/base.h"
  echo "mini_chromium BSD" > "$root/third_party/mini_chromium/mini_chromium/LICENSE"
  printf 'Name: mini_chromium\nLicense: BSD 3-clause\nLicense File: mini_chromium/LICENSE\n' \
    > "$root/third_party/mini_chromium/README.crashpad"
  # CRLF, as a Windows checkout may have it.
  printf 'Name: zlib\r\nLicense: zlib\r\nLicense File: zlib/LICENSE\r\n' > "$root/third_party/zlib/README.crashpad"
  echo "zlib license" > "$root/third_party/zlib/zlib/LICENSE"
  echo "lib" > "$root/out/test/obj/client/libclient.a"
  echo "lib" > "$root/out/test/obj/third_party/zlib/libzlib.a"
  echo 'is_debug=false' > "$root/out/test/args.gn"
  git -C "$root" init -q
  git -C "$root" -c user.name=t -c user.email=t@example.com commit -q --allow-empty -m fake
}

# Ships Crashpad's license and every shipped third-party component's license.
ROOT="$WORK/ok/crashpad"
fake_checkout "$ROOT"
bash "$HERE/package.sh" "$ROOT" test fake "$WORK/ok/dist" > "$WORK/ok.log" 2>&1 || { cat "$WORK/ok.log"; fail "package.sh failed"; }
TARBALL=$(ls "$WORK/ok/dist"/*.tar.xz 2>/dev/null | head -1)
if [ -n "$TARBALL" ]; then
  mkdir -p "$WORK/ok/x" && tar -C "$WORK/ok/x" -xJf "$TARBALL"
  PKG="$WORK/ok/x/crashpad"
  grep -q "Apache License" "$PKG/LICENSE" || fail "Crashpad LICENSE missing"
  grep -q "mini_chromium BSD" "$PKG/third_party_licenses/mini_chromium/LICENSE" || fail "mini_chromium LICENSE missing"
  grep -q "zlib license" "$PKG/third_party_licenses/zlib/LICENSE" || fail "zlib LICENSE missing"
  grep -qx "mini_chromium: BSD 3-clause (mini_chromium/LICENSE)" "$PKG/third_party_licenses/README" \
    || fail "README does not list mini_chromium"
  grep -qx "zlib: zlib (zlib/LICENSE)" "$PKG/third_party_licenses/README" || fail "README does not list zlib"
  [ "$(wc -l < "$PKG/third_party_licenses/README")" -eq 2 ] || fail "README lists unexpected components"
else
  fail "no tarball produced"
fi

# Refuses to package a shipped component whose license file it cannot find.
ROOT="$WORK/missing/crashpad"
fake_checkout "$ROOT"
rm "$ROOT/third_party/zlib/zlib/LICENSE"
if bash "$HERE/package.sh" "$ROOT" test fake "$WORK/missing/dist" > "$WORK/missing.log" 2>&1; then
  fail "package.sh succeeded without zlib's license"
elif ! grep -q "third_party/zlib is shipped but has no license file" "$WORK/missing.log"; then
  cat "$WORK/missing.log"; fail "unexpected error for a missing license"
fi

# Checksums with shasum where sha256sum is missing, as on stock macOS.
if command -v shasum >/dev/null 2>&1; then
  ROOT="$WORK/shasum/crashpad"
  fake_checkout "$ROOT"
  mkdir -p "$WORK/shasum/bin"
  IFS=: read -ra dirs <<< "$PATH"
  for d in "${dirs[@]}"; do
    for exe in "$d"/*; do
      name=$(basename "$exe")
      if [ -x "$exe" ] && [ "$name" != sha256sum ] && [ ! -e "$WORK/shasum/bin/$name" ]; then
        ln -s "$exe" "$WORK/shasum/bin/$name"
      fi
    done
  done
  if PATH="$WORK/shasum/bin" bash "$HERE/package.sh" "$ROOT" test fake "$WORK/shasum/dist" > "$WORK/shasum.log" 2>&1; then
    (cd "$WORK/shasum/dist" && shasum -a 256 -c -- *.sha256 > /dev/null) || fail "shasum checksum does not verify"
  else
    cat "$WORK/shasum.log"; fail "package.sh failed without sha256sum"
  fi
fi

if [ "$FAILED" -ne 0 ]; then exit 1; fi
echo "package.sh tests passed"
