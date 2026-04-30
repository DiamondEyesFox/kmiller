#!/usr/bin/env bash
set -euo pipefail

fixture="${1:-$HOME/Downloads/Kmiller Testing Folder}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

exec python3 "$script_dir/kmiller_manual_qa_harness.py" "$fixture"
