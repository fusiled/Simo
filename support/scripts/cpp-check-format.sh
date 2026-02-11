#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "Error: clang-format is not installed or not in PATH." >&2
  exit 2
fi

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
AUTO_FORMAT=0

usage() {
  cat <<'EOF'
Usage: ./check-format.sh [--auto-format]

Options:
  --auto-format   Format matching files in place
  -h, --help      Show this help message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --auto-format)
      AUTO_FORMAT=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Error: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

status=0

while IFS= read -r -d '' file; do
  if [[ "$AUTO_FORMAT" -eq 1 ]]; then
    "$CLANG_FORMAT" -i "$file"
    echo "Formatted: $file"
    continue
  fi

  if ! diff -u "$file" <("$CLANG_FORMAT" "$file") >/dev/null; then
    echo "Not formatted: $file"
    status=1
  fi
done < <(find . -type f \( -name '*.cc' -o -name '*.h' \) -print0)

if [[ "$AUTO_FORMAT" -eq 1 ]]; then
  echo "Formatting complete."
  exit 0
fi

if [[ "$status" -eq 0 ]]; then
  echo "All .cc and .h files are properly formatted."
else
  echo "Some files are not properly formatted."
fi

exit "$status"
