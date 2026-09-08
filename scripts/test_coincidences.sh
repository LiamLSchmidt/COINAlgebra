#!/usr/bin/env bash
# Rebuild and run only the DecayCoin/CAlgebra regression tests.
# Usage: scripts/test_coincidences.sh [additional ctest arguments]
set -euo pipefail
coin_source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
coin_build_dir="$coin_source_dir/build"
if [[ ! -f "$coin_build_dir/CMakeCache.txt" ]]; then
    echo "Configure first with scripts/build.sh (optionally -DBUILD_WITH_ROOT=OFF)." >&2
    exit 1
fi
coin_targets=(test_native_coin)
if rg -q '^BUILD_WITH_ROOT:BOOL=ON$' "$coin_build_dir/CMakeCache.txt"; then
    coin_targets+=(coinalgebra)
fi
cmake --build "$coin_build_dir" --target "${coin_targets[@]}" --parallel 4
ctest --test-dir "$coin_build_dir" -R '^(native\.coin|root\.test_coin)$' \
    --verbose --output-on-failure "$@"
