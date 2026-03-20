#!/usr/bin/env bash
set -euo pipefail

if ! command -v pacman >/dev/null 2>&1; then
  echo "pacman is required on Arch Linux." >&2
  exit 1
fi

sudo pacman -Sy --needed base-devel bash doxygen graphviz

echo "Arch Linux dependencies installed. You can now build with 'make test' or generate docs with 'make docs'."
