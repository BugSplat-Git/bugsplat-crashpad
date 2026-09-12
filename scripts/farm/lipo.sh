#!/usr/bin/env bash
# Merge two single-architecture GN out dirs into one universal out dir (macOS, iOS simulator,
# tvOS simulator): every static library and the tool_support object are lipo'd, generated
# headers and args.gn come from the first input.
#
#   lipo.sh <crashpad-root> <out-a> <out-b> <out-universal>
set -euo pipefail
ROOT=$1; A=$2; B=$3; U=$4
cd "$ROOT/out"
rm -rf "$U"; mkdir -p "$U/obj"
(cd "$A" && find obj -type f \( -name '*.a' -o -name 'tool_support.tool_support.o' \) -print) | while read -r f; do
  mkdir -p "$U/$(dirname "$f")"
  if [ -f "$B/$f" ]; then
    lipo -create "$A/$f" "$B/$f" -output "$U/$f"
  else
    echo "warning: $f only exists in $A" >&2
    cp "$A/$f" "$U/$f"
  fi
done
[ -d "$A/gen" ] && cp -R "$A/gen" "$U/gen"
{ echo "# universal: $A + $B"; cat "$A/args.gn"; } > "$U/args.gn"
if [ -f "$A/crashpad_handler" ] && [ -f "$B/crashpad_handler" ]; then
  lipo -create "$A/crashpad_handler" "$B/crashpad_handler" -output "$U/crashpad_handler"
fi
lipo -info "$U/obj/client/libclient.a"
