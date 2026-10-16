#!/bin/sh
# Build the ROM from a source snapshot inside a persistent work tree, so unchanged
# objects and converted graphics are reused between `docker build` runs.
#
# usage: build-rom.sh SRC WORK OUT
#   SRC   source snapshot (the image's /src)
#   WORK  cache-mounted work tree that keeps build outputs between runs
#   OUT   where the ROM is copied, as EliteRedux.gba
set -eu

src=$1
work=$2
out=$3
manifest="${work}/.source-manifest"

if [ "${INCREMENTAL:-1}" = 0 ]; then
    find "${work}" -mindepth 1 -delete
fi

# Remove files that were in the previous snapshot but are gone now. Build outputs
# were never in a snapshot, so they are kept.
(cd "${src}" && find . \( -type f -o -type l \) | LC_ALL=C sort) > /tmp/source-manifest
if [ -f "${manifest}" ]; then
    LC_ALL=C comm -23 "${manifest}" /tmp/source-manifest | (cd "${work}" && xargs -r -d '\n' rm -f --)
fi

# Compare by content and don't copy mtimes: only files that really changed are
# rewritten, and they get the current time, so make rebuilds exactly what depends on
# them whatever the host timestamps were (other branches, other checkouts, ...).
rsync --recursive --links --perms --checksum --out-format='%n' "${src}/" "${work}/" > /tmp/rsync.log
grep -v '/$' /tmp/rsync.log > /tmp/synced || true
mv /tmp/source-manifest "${manifest}"
echo "Synced $(wc -l < /tmp/synced) changed source files"
head -n 20 /tmp/synced

cd "${work}"
jobs=${JOBS:-$(nproc)}
# The codegen stage already generated the headers, so build the other tools here...
tooldirs=$(for dir in tools/*/; do
    case "${dir}" in
        tools/codegen/|tools/agbcc/|tools/binutils/|tools/poryscript/) ;;
        *) printf '%s ' "${dir%/}" ;;
    esac
done)
make -f make_tools.mk -j"${jobs}" TOOLDIRS="${tooldirs}"
# ...and ask for the ROM file itself: the `all`/`rom` goals would rebuild every tool,
# codegen included, and rewrite every generated header. NODEP=0 keeps header
# dependency scanning on, which that goal would otherwise switch off.
make -j"${jobs}" NODEP=0 pokeemerald_modern.gba
mkdir -p "${out}"
cp pokeemerald_modern.gba "${out}/EliteRedux.gba"
