#!/usr/bin/env bash
set -euo pipefail
coin_source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$coin_source_dir" -B "$coin_source_dir/build" "$@"
cmake --build "$coin_source_dir/build" --parallel 4
