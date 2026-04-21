#!/bin/bash

set -euo pipefail

preset="${1:-debug}"

case "$preset" in
  debug|relwithdebinfo|release)
    ;;
  *)
    echo "Usage: $0 [debug|release]" >&2
    exit 1
    ;;
esac

cmake --preset "$preset"
cmake --build --preset "$preset" --parallel
