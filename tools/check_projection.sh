#!/usr/bin/env bash
# Runs every check: unit tests, the in-engine self-test, and the Python
# reference comparison.
set -e

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

echo "== unit tests =="
(cd tests/PoseProjection.Tests && dotnet test --nologo)

echo "== build =="
dotnet build PoseProjection.csproj --nologo

echo "== in-engine self-test =="
godot-mono --headless --path . -- --self-test

echo "== export tables =="
godot-mono --headless --path . -- --export-csv

echo "== python reference =="
python3 tools/reference_projection.py

echo "all checks passed"
