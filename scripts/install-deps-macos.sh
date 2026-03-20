#!/usr/bin/env bash
set -euo pipefail

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS. Install it from https://brew.sh and rerun 'make deps'." >&2
  exit 1
fi

if ! xcode-select -p >/dev/null 2>&1; then
  echo "Installing Xcode Command Line Tools..."
  xcode-select --install || true
  echo "If macOS opened an installer dialog, finish that installation and rerun 'make deps'." >&2
fi

brew update
brew install bash make gcc doxygen graphviz

echo "macOS dependencies installed. You can now build with 'make test' or generate docs with 'make docs'."
