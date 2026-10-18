#!/bin/sh
# run_hotreload_tests.sh (2026-10-18) -- the hot-reload shim's Linux tests: logic (fake builds) + end to end (real .so builds, real files)
set -e
cd "$(dirname "$0")"
OUT=${TMPDIR:-/tmp}/hotreload_tests.$$
mkdir -p "$OUT"
F="-std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I."
g++ $F test_hotreload_logic.cpp -o "$OUT/logic" && "$OUT/logic"
S="-std=c++17 -Werror -Wall -Wextra -I. -fPIC -shared"
g++ $S -DHR_BUILD=1 -DE2E_VERSION='"1.0.0"' test_hotreload_e2e_impl.cpp -o "$OUT/lsHotReloadDemo_impl_1.so"
g++ $S -DHR_BUILD=2 -DE2E_VERSION='"1.0.1"' test_hotreload_e2e_impl.cpp -o "$OUT/lsHotReloadDemo_impl_2.so"
g++ $S -DHR_BUILD=3 -DE2E_VERSION='"1.1.0"' -DE2E_PARAMS='"v2|output:Hot Reload Demo|bool:Show line"' test_hotreload_e2e_impl.cpp -o "$OUT/lsHotReloadDemo_impl_3.so"
g++ -std=c++17 -Werror -Wall -Wextra -I. test_hotreload_e2e.cpp -o "$OUT/e2e" -ldl
"$OUT/e2e" "$OUT"
rm -rf "$OUT"
