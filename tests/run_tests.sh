#!/bin/sh
# Run parser/semantic cases listed in a manifest.
# Manifest rows: <source>|<expected exit status>|<expected output regex>

set -u

if [ "$#" -ne 2 ] && [ "$#" -ne 3 ]; then
  printf '%s\n' "Usage: $0 <parser> <manifest> [--tree]" >&2
  exit 2
fi

. "$(dirname "$0")/lib.sh"

parser=$1
manifest=$2
mode=${3-}
init_counters "$(suite_name_for "$manifest" cases)"

while IFS='|' read -r test_file expected_status expected_pattern; do
  is_skipped_line "$test_file" && continue

  total=$((total + 1))
  output_file=$(make_temp output)

  if [ "$mode" = "--tree" ]; then
    "$parser" --tree "$test_file" >"$output_file" 2>&1
  else
    "$parser" "$test_file" >"$output_file" 2>&1
  fi
  actual_status=$?

  output_matches=0
  if [ "$expected_pattern" = "__EMPTY__" ]; then
    [ ! -s "$output_file" ] && output_matches=1
  elif grep -Eq "$expected_pattern" "$output_file"; then
    output_matches=1
  fi

  if [ "$actual_status" -eq "$expected_status" ] &&
    [ "$output_matches" -eq 1 ]; then
    passed=$((passed + 1))
  else
    has_failure=1
    printf '%s: FAIL %s\n' "$suite_name" "$test_file"
    printf '  expected exit status: %s\n' "$expected_status"
    printf '  expected output pattern: %s\n' "$expected_pattern"
    printf '  actual exit status: %s\n' "$actual_status"
    printf '%s\n' '  actual output:'
    print_indented "$output_file"
  fi

  rm -f "$output_file"
done <"$manifest"

finish_suite
