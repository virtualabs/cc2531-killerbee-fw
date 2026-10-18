#!/bin/sh
# Apply patches/contiki/*.patch to the contiki submodule checkout, for an
# in-tree `make`. Safe to re-run: patches already applied are skipped.
# (scripts/build.sh does not need this; it patches its own copy.)
set -eu
cd "$(dirname "$0")/.."
[ -f contiki/Makefile.include ] ||
  { echo "contiki submodule missing: git submodule update --init contiki" >&2; exit 1; }
for p in patches/contiki/*.patch; do
  # Check "already applied" first: a patch that only inserts lines (0002)
  # still applies forward on top of itself.
  if git -C contiki apply -R --check "../$p" 2>/dev/null; then
    echo "present  $p"
  elif git -C contiki apply --check "../$p" 2>/dev/null; then
    git -C contiki apply "../$p" && echo "applied  $p"
  else
    echo "does not apply: $p" >&2; exit 1
  fi
done
