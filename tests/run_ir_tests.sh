#!/bin/sh
# Compile cases to IR and compare with the expected IR files.
# Manifest rows: <source>|<expected IR file>

set -u

if [ "$#" -ne 2 ]; then
	printf '%s\n' "Usage: $0 <parser> <manifest>" >&2
	exit 2
fi

. "$(dirname "$0")/lib.sh"

parser=$1
manifest=$2
init_counters "$(suite_name_for "$manifest" cases)"

while IFS='|' read -r source_file expected_ir; do
	is_skipped_line "$source_file" && continue

	total=$((total + 1))
	actual_ir=$(make_temp ir)
	diagnostic_output=$(make_temp diagnostic)

	"$parser" "$source_file" "$actual_ir" >"$diagnostic_output" 2>&1
	actual_status=$?

	if [ "$actual_status" -eq 0 ] && [ ! -s "$diagnostic_output" ] &&
		cmp -s "$expected_ir" "$actual_ir"; then
		passed=$((passed + 1))
	else
		has_failure=1
		printf '%s: FAIL %s\n' "$suite_name" "$source_file"
		printf '%s\n' '  expected exit status: 0'
		printf '  expected IR file: %s\n' "$expected_ir"
		printf '  actual exit status: %s\n' "$actual_status"
		printf '%s\n' '  diagnostic output:'
		print_indented "$diagnostic_output"
		printf '%s\n' '  actual IR:'
		print_indented "$actual_ir"
	fi

	rm -f "$actual_ir" "$diagnostic_output"
done <"$manifest"

finish_suite
