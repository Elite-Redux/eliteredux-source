#!/bin/sh
# make-sym.sh ELF ROM > SYM: the ROM's functions and variables, so testing tools don't need the ELF.
# Lines are `ADDRESS SIZE KIND BIND NAME`, keeping the Thumb bit and static symbols.
set -eu
echo "# sparring symbols 1"
echo "# rom sha1 $(sha1sum < "$2" | cut -d' ' -f1)"
arm-none-eabi-readelf -sW "$1" | awk '($4 == "FUNC" || $4 == "OBJECT") && $2 ~ /^0[2389]/ { print $2, $3, substr($4, 1, 1), tolower(substr($5, 1, 1)), $8 }'
