#!/usr/bin/env bash
# Verifies the keymap macro reorders the visual layout into the electrical
# matrix correctly. Expands keymap.h with the preprocessor and checks the
# resulting tables, so it needs no host compiler or emulator.
#
# Usage: ./check_keymap.sh
set -euo pipefail
cd "$(dirname "$0")"

TC=.platformio/packages/toolchain-riscv/bin/riscv-wch-elf-gcc
[ -x "$TC" ] || { echo "toolchain missing; run 'pio run' first"; exit 1; }

tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
printf '#include "keycodes.h"\n#include "keymap.h"\n' > "$tmp/e.c"
map=$("$TC" -E -P -I src "$tmp/e.c" | tr -d ' ' | grep -oE '\[BASE\]=\{.*?\}\}' | head -1)

fail=0
expect() { # expect <row> <description> <hex bytes...>
  local row=$1 desc=$2; shift 2
  # Bare hex bytes get an 0x prefix; anything already containing '(' or '0x'
  # (an unevaluated LAYER() macro) is used verbatim.
  local want=""
  for v in "$@"; do
    case "$v" in
      *'('*|0x*) want="$want,$v" ;;
      *)         want="$want,0x$v" ;;
    esac
  done
  want=$(echo "${want#,}" | tr 'A-Z' 'a-z')
  local got; got=$(echo "$map" | grep -oE '\{[^{}]*\}' | sed -n "$((row+1))p" \
                   | tr -d '{}' | tr 'A-Z' 'a-z')
  if [ "$got" = "$want" ]; then
    echo "  ok   row$row  $desc"
  else
    echo "  FAIL row$row  $desc"; echo "       want: $want"; echo "       got:  $got"
    fail=1
  fi
}

echo "BASE layer, electrical matrix:"
# Left half, rows 0-2: visual order is preserved.
expect 0 "Tab Q W E R T"          2b 14 1a 08 15 17
expect 1 "Ctl A S D F G"          e0 04 16 07 09 0a
expect 2 "Sft Z X C V B"          e1 1d 1b 06 19 05
# Right half, rows 4-6: same column order as the left, col0 is the inner column.
expect 4 "Y U I O P Bsp"          1c 18 0c 12 13 2a
expect 5 "H J K L ; '"            0b 0d 0e 0f 33 34
expect 6 "N M , . / Ent"          11 10 36 37 38 28
# Thumb rows: only cols 3-5 are populated.
expect 3 "-- -- -- Esc SYM Spc"      00 00 00 29 "(0xf0|(sym))" 2c
expect 7 "-- -- -- Sft* NUM Alt*"    00 00 00 "((0xe1)|0x08)" "(0xf0|(num))" "((0xe2)|0x08)"

# The boot combo must be reachable: Q and A on the same column, adjacent rows.
echo "Boot combo (Q+A):"
if [ "$(echo "$map" | grep -oE '\{[^{}]*\}' | sed -n 1p | cut -d, -f2)" = "0x14" ] &&
   [ "$(echo "$map" | grep -oE '\{[^{}]*\}' | sed -n 2p | cut -d, -f2)" = "0x04" ]; then
  echo "  ok   Q at [0][1], A at [1][1]"
else
  echo "  FAIL boot combo moved; update BOOT_KEY_* in src/main.cpp"; fail=1
fi

[ $fail -eq 0 ] && echo "PASS" || { echo "FAIL"; exit 1; }
