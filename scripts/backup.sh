#!/usr/bin/env bash
# Snapshot the ledger database and keep the newest N backups.
set -euo pipefail

DB="${1:-var/ledger.db}"
DEST="${2:-backups}"
KEEP="${KEEP:-14}"

[[ -f "$DB" ]] || { echo "no database at $DB" >&2; exit 1; }
command -v sqlite3 >/dev/null || { echo "sqlite3 is required" >&2; exit 1; }

mkdir -p "$DEST"
stamp="$(date +%Y%m%d-%H%M%S)"
out="$DEST/ledger-$stamp.sqlite"

sqlite3 "$DB" ".backup '$out'"
sqlite3 "$out" "PRAGMA integrity_check;" | grep -q '^ok$'

gzip -9 "$out"
echo "wrote $out.gz ($(du -h "$out.gz" | cut -f1))"

ls -1t "$DEST"/ledger-*.sqlite.gz 2>/dev/null | tail -n +$((KEEP + 1)) | while IFS= read -r f; do
  rm -f "$f"
  echo "pruned $f"
done
