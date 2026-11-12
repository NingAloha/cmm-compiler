#!/bin/sh
# Check that unsupported inputs fail and produce no IR file.
# Manifest rows: <source>

set -u

if [ "$#" -ne 2 ]; then
  printf '%s\n' "Usage: $0 <parser> <manifest>" >&2
  exit 2
fi

. "$(dirname "$0")/lib.sh"

parser=$1
manifest=$2
init_counters "$(suite_name_for "$manifest" rejections)"

while IFS= read -r source_file; do
  is_skipped_line "$source_file" && continue

  total=$((total + 1))
  output_file=$(make_temp rejected-ir)
  diagnostic_output=$(make_temp diagnostic)
  rm -f "$output_file"

  "$parser" "$source_file" "$output_file" >"$diagnostic_output" 2>&1
  actual_status=$?

  if [ "$actual_status" -ne 0 ] && [ ! -e "$output_file" ]; then
    passed=$((passed + 1))
  else
    has_failure=1
    if [ -e "$output_file" ]; then ir_exists=yes; else ir_exists=no; fi
    printf '%s: FAIL %s\n' "$suite_name" "$source_file"
    printf '%s\n' '  expected a nonzero exit status and no IR file'
    printf '  actual exit status: %s\n' "$actual_status"
    printf '  IR file exists: %s\n' "$ir_exists"
    printf '%s\n' '  diagnostic output:'
    print_indented "$diagnostic_output"
  fi

  rm -f "$output_file" "$diagnostic_output"
done <"$manifest"

finish_suite
