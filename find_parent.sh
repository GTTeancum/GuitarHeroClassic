#!/bin/bash
# Usage: find_parent.sh 0x826DED50
# Finds the legacy function in gh2test_config.toml that contains the given address.
addr_hex="${1#0x}"
addr=$(printf "%d" "0x$addr_hex")
awk -F'[ =]+' -v target="$addr" '
/^0x[0-9a-fA-F]+ = \{ name = / {
  gsub(/^0x/, "", $1)
  start = strtonum("0x" $1)
  # size = "0x..." is field 6 (split by = and spaces)
  for (i = 1; i <= NF; i++) {
    if ($i ~ /^0x[0-9a-fA-F]+$/ && i > 4) {
      sz = strtonum($i)
      break
    }
  }
  end = start + sz
  if (target >= start && target < end) {
    printf "0x%08X size=0x%X (covers offset +0x%X)\n", start, sz, target - start
    exit 0
  }
}' "C:/Programming/GitHub/Guitar Hero II/GuitarHeroOGX/gh2test_config.toml"
