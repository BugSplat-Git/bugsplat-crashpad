#!/usr/bin/env bash
# Merges two single-architecture out dirs into a universal one. Generated headers and args.gn
# come from <out-a>.
#
#   lipo.sh <crashpad-root> <out-a> <out-b> <out-universal>
set -euo pipefail
ROOT=$1; A=$2; B=$3; U=$4
cd "$ROOT/out"
rm -rf "$U"; mkdir -p "$U/obj"
objects() { (cd "$1" && find obj -type f \( -name '*.a' -o -name 'tool_support.tool_support.o' \) -print); }
{ objects "$A"; objects "$B"; } | sort -u | while read -r f; do
  if [ ! -f "$A/$f" ] || [ ! -f "$B/$f" ]; then
    echo "lipo.sh: $f was built for only one of $A and $B" >&2
    exit 1
  fi
  mkdir -p "$U/$(dirname "$f")"
  lipo -create "$A/$f" "$B/$f" -output "$U/$f"
done
[ -d "$A/gen" ] && cp -R "$A/gen" "$U/gen"
{ echo "# universal: $A + $B"; cat "$A/args.gn"; } > "$U/args.gn"
if [ -f "$A/crashpad_handler" ] && [ -f "$B/crashpad_handler" ]; then
  lipo -create "$A/crashpad_handler" "$B/crashpad_handler" -output "$U/crashpad_handler"
fi
lipo -info "$U/obj/client/libclient.a"
