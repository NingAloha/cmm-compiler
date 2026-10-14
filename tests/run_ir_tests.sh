#!/bin/sh

set -u

if [ "$#" -ne 2 ]; then
    printf '%s\n' "Usage: $0 <parser> <manifest>" >&2
    exit 2
fi

parser=$1
manifest=$2
stage_name=$(basename "$(dirname "$manifest")")
suite_name="$stage_name/cases"
passed=0
total=0
has_failure=0

while IFS='|' read -r source_file expected_ir; do
    case "$source_file" in
    '' | \#*)
        continue
        ;;
    esac

    total=$((total + 1))
    actual_ir=$(mktemp "${TMPDIR:-/tmp}/cmm-compiler-ir.XXXXXX") || exit 2
    diagnostic_output=$(mktemp "${TMPDIR:-/tmp}/cmm-compiler-diagnostic.XXXXXX") || exit 2

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
        sed 's/^/    /' "$diagnostic_output"
        printf '%s\n' '  actual IR:'
        sed 's/^/    /' "$actual_ir"
    fi

    rm -f "$actual_ir" "$diagnostic_output"
done <"$manifest"

printf '%s: %d/%d passed\n' "$suite_name" "$passed" "$total"
exit "$has_failure"
