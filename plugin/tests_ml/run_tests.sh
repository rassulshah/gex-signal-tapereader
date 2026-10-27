#!/usr/bin/env bash
# MLInfer.h tests: build the parity fixture from a real exported model (Python), compile + run the C++ test.
# usage: tests_ml/run_tests.sh <path to level-reversal-analytics/analytics>
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ANALYTICS="${1:-$HERE/../../../level-reversal-analytics/analytics}"
OUT="$HERE/out"
mkdir -p "$OUT"
python "$HERE/make_fixture.py" "$ANALYTICS" "$OUT"
g++ -std=c++17 -O2 -Wall -Wextra -o "$OUT/test_mlinfer" "$HERE/test_mlinfer.cpp"
"$OUT/test_mlinfer" "$OUT"
