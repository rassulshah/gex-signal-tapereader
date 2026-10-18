#!/bin/sh
# Settings layer tests (Linux): the contract over every table + the host test on the IRT-behaviour mock. Run before staging any plugin.
set -e
cd "$(dirname "$0")"
OUT=${TMPDIR:-/tmp}
g++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -o "$OUT/settings_contract" settings_contract.cpp
g++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -o "$OUT/settings_host_test" settings_host_test.cpp
"$OUT/settings_contract" ..
"$OUT/settings_host_test"
# the gate must catch a hand-written colour call (proof the contract fails when it should)
T=$(mktemp -d); cp ../*.h ../*.cpp "$T"/ 2>/dev/null || true
sed -i 's|ss::registerAll(this, hlset::spec(), HodLod::hlIds());|ss::registerAll(this, hlset::spec(), HodLod::hlIds()); setIntegerValue(4, 0);|' "$T/HodLod.cpp"
if "$OUT/settings_contract" "$T" > "$T/out.txt"; then echo "FAIL the contract did not catch setIntegerValue"; exit 1; else grep -c "calls setIntegerValue" "$T/out.txt" >/dev/null && echo "PASS the contract catches a hand-written setIntegerValue"; fi
rm -rf "$T"
