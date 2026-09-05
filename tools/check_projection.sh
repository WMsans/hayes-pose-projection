#!/usr/bin/env bash
# Runs every check: unit tests, the in-engine self-test, and the Python
# reference comparison.
set -euo pipefail

run_clean_godot() {
    local log status=0
    log="$(mktemp)"
    godot-mono "$@" 2>&1 | tee "$log" || status="${PIPESTATUS[0]}"
    if [[ "$status" -ne 0 ]]; then
        rm -f "$log"
        return "$status"
    fi
    if grep -q '^ERROR:' "$log"; then
        echo "Godot reported errors during: godot-mono $*" >&2
        rm -f "$log"
        return 1
    fi
    rm -f "$log"
}

check_table() {
    local path="$1" expected_lines="$2"
    test -s "$path" || { echo "missing or empty export: $path" >&2; return 1; }
    local lines
    lines="$(wc -l < "$path")"
    test "$lines" -eq "$expected_lines" || {
        echo "expected $expected_lines lines in $path, found $lines" >&2
        return 1
    }
}

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

echo "== unit tests =="
(cd tests/PoseProjection.Tests && dotnet test --nologo)

echo "== build =="
dotnet build PoseProjection.csproj --nologo

echo "== in-engine self-test =="
run_clean_godot --headless --path . -- --self-test

echo "== export tables =="
run_clean_godot --headless --path . -- --export-csv
check_table out/coords_wide.csv 21
check_table out/coords_long.csv 561
check_table out/method_agreement.csv 21

echo "== python reference =="
python3 tools/reference_projection.py

echo "all checks passed"
