#!/usr/bin/env bash
set -euo pipefail
coin_source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ctest --test-dir "$coin_source_dir/build" --output-on-failure "$@"
