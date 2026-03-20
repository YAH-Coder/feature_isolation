#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
  echo "apt-get is required on Debian-based systems." >&2
  exit 1
fi

sudo apt-get update
sudo apt-get install -y build-essential bash make g++ doxygen graphviz

echo "Debian-family dependencies installed. You can now build with 'make test' or generate docs with 'make docs'."
