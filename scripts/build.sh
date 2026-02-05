#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
cd "$REPO_ROOT"

config="${1:-debug}"
case "$config" in
  debug|release) ;;
  *)
    echo "Usage: $0 [debug|release]"
    exit 1
    ;;
esac

cmake --preset "$config"
cmake --build --preset "$config"
