#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNAME_S="$(uname -s)"

case "${UNAME_S}" in
  Darwin)
    exec bash "${SCRIPT_DIR}/install-deps-macos.sh"
    ;;
  Linux)
    if [[ ! -r /etc/os-release ]]; then
      echo "Cannot determine Linux distribution because /etc/os-release is missing." >&2
      exit 1
    fi

    # shellcheck disable=SC1091
    . /etc/os-release

    case "${ID:-}" in
      ubuntu)
        exec bash "${SCRIPT_DIR}/install-deps-ubuntu.sh"
        ;;
      debian)
        exec bash "${SCRIPT_DIR}/install-deps-debian.sh"
        ;;
      arch|archlinux)
        exec bash "${SCRIPT_DIR}/install-deps-arch.sh"
        ;;
      *)
        if [[ "${ID_LIKE:-}" == *debian* ]]; then
          exec bash "${SCRIPT_DIR}/install-deps-debian.sh"
        elif [[ "${ID_LIKE:-}" == *arch* ]]; then
          exec bash "${SCRIPT_DIR}/install-deps-arch.sh"
        else
          echo "Unsupported Linux distribution: ID='${ID:-unknown}', ID_LIKE='${ID_LIKE:-unknown}'." >&2
          echo "Supported platforms: macOS, Debian, Ubuntu, Arch Linux, and Windows." >&2
          exit 1
        fi
        ;;
    esac
    ;;
  MINGW*|MSYS*|CYGWIN*)
    echo "Windows shell detected. Run 'make deps' from the repository root so the Makefile can invoke the PowerShell bootstrap." >&2
    exit 1
    ;;
  *)
    echo "Unsupported operating system: ${UNAME_S}" >&2
    echo "Supported platforms: macOS, Debian, Ubuntu, Arch Linux, and Windows." >&2
    exit 1
    ;;
esac
