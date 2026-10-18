#!/bin/sh
# Reproducible build of the CC2531 Bumblebee firmware with Debian's SDCC 4.5.0
# (arm64), run from extracted .debs: nothing is installed, no root needed.
#
# usage: scripts/build.sh WORKDIR [REV]
#   WORKDIR  new or empty directory: toolchain, sources and build log go here
#   REV      commit to build (default HEAD)
# Output: WORKDIR/fw/cc2531-bumblebee.hex; its sha256 is printed on stdout.
#
# Only committed sources are built: REV is exported with git archive, the
# Contiki submodule is fetched at the commit REV pins, and REV's
# patches/contiki/*.patch are applied in order. Uncommitted changes are ignored.
# Expected hashes are in README.md. SDCC_BIN=/dir/with/sdcc uses another SDCC
# instead of the pinned one; the hex may then differ.
set -eu
[ $# -ge 1 ] || { echo "usage: $0 WORKDIR [REV]" >&2; exit 2; }
W=$1; REV=${2:-HEAD}
REPO=$(cd "$(dirname "$0")/.." && pwd)
[ -d "$W" ] && [ -n "$(ls -A "$W")" ] && { echo "workdir must be empty: $W" >&2; exit 1; }
mkdir -p "$W" && W=$(cd "$W" && pwd)

SHA=$(git -C "$REPO" rev-parse --verify "$REV^{commit}")
CONTIKI_SHA=$(git -C "$REPO" ls-tree "$SHA" contiki | awk '$2 == "commit" { print $3 }')
CONTIKI_URL=$(git -C "$REPO" config --blob "$SHA:.gitmodules" submodule.contiki.url)
[ -n "$CONTIKI_SHA" ] || { echo "no contiki submodule in $REV" >&2; exit 1; }

if [ -n "${SDCC_BIN:-}" ]; then
  echo "warning: using $SDCC_BIN/sdcc, not the pinned SDCC 4.5.0; the hex may differ" >&2
  PATH=$SDCC_BIN:$PATH
else
  [ "$(dpkg --print-architecture 2>/dev/null)" = arm64 ] ||
    { echo "the pinned SDCC debs are arm64; on another host set SDCC_BIN" >&2; exit 1; }
  mkdir "$W/debs" && (cd "$W/debs" &&
    curl -sfL -o sdcc_4.5.0+dfsg-1_arm64.deb https://snapshot.debian.org/file/39e7cf050068a79fdb6d1b95a326dc6bbc60af6b &&
    curl -sfL -o sdcc-libraries_4.5.0+dfsg-1_all.deb https://snapshot.debian.org/file/dd4899d042cd9a16111c7f7b90bc4f1e586bcb2a &&
    sha256sum -c --quiet - <<SUMS
3d8effe952ca44695008c4fce95d528c724b21c82b6210acb342ad01722c7d94  sdcc_4.5.0+dfsg-1_arm64.deb
3693c67f573663d400d95a2bad8d41ba1047decb608f4ee61d9d2e2fa96046a7  sdcc-libraries_4.5.0+dfsg-1_all.deb
SUMS
  )
  mkdir "$W/sdcc" && for d in "$W"/debs/*.deb; do dpkg-deb -x "$d" "$W/sdcc"; done
  PATH=$W/sdcc/usr/bin:$PATH
fi
export PATH

# Sources: REV, plus Contiki at the pinned commit (reuse the local submodule
# checkout when it has that commit, else a shallow fetch).
mkdir "$W/fw" && git -C "$REPO" archive "$SHA" | tar -x -C "$W/fw"
mkdir -p "$W/fw/contiki"
if git -C "$REPO/contiki" cat-file -e "$CONTIKI_SHA^{commit}" 2>/dev/null; then
  git -C "$REPO/contiki" archive "$CONTIKI_SHA" | tar -x -C "$W/fw/contiki"
else
  git init -q --bare "$W/contiki.git"
  git -C "$W/contiki.git" fetch -q --depth 1 "$CONTIKI_URL" "$CONTIKI_SHA"
  git -C "$W/contiki.git" archive "$CONTIKI_SHA" | tar -x -C "$W/fw/contiki"
fi
for p in "$W"/fw/patches/contiki/*.patch; do
  [ -e "$p" ] || continue
  patch -s -p1 -F0 -N --no-backup-if-mismatch -d "$W/fw/contiki" < "$p"
done

echo "building $(git -C "$REPO" describe --tags --always "$SHA") with $(sdcc -v | head -1)" >&2
# Contiki embeds RELSTR (default: git describe of contiki/) as its version
# string; pin it to the submodule's short hash so the image does not depend
# on how contiki/ was checked out.
make -C "$W/fw" RELSTR="$(printf %.7s "$CONTIKI_SHA")" > "$W/build.log" 2>&1 ||
  { tail -20 "$W/build.log"; exit 1; }
cd "$W/fw" && sha256sum cc2531-bumblebee.hex
