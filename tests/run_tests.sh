#!/bin/sh

set -u

if [ "$#" -ne 2 ] && [ "$#" -ne 3 ]; then
    printf '%s\n' "Usage: $0 <parser> <manifest> [--semantic]" >&2
    exit 2
fi

parser=$1
manifest=$2
mode=${3-}
stage_name=$(basename "$(dirname "$manifest")")
passed=0
total=0
has_failure=0

while IFS='|' read -r test_file expected_status expected_pattern; do
    case "$test_file" in
    '' | \#*)
        continue
        ;;
    esac

    total=$((total + 1))
    output_file=$(mktemp "${TMPDIR:-/tmp}/cmm-compiler-test.XXXXXX") || exit 2

    if [ "$mode" = "--semantic" ]; then
        "$parser" --semantic "$test_file" >"$output_file" 2>&1
    else
        "$parser" "$test_file" >"$output_file" 2>&1
    fi
    actual_status=$?

    if [ "$actual_status" -eq "$expected_status" ] &&
        grep -Eq "$expected_pattern" "$output_file"; then
        passed=$((passed + 1))
    else
        has_failure=1
        printf '%s: FAIL %s\n' "$stage_name" "$test_file"
        printf '  expected exit status: %s\n' "$expected_status"
        printf '  expected output pattern: %s\n' "$expected_pattern"
        printf '  actual exit status: %s\n' "$actual_status"
        printf '  actual output:\n'
        sed 's/^/    /' "$output_file"
    fi

    rm -f "$output_file"
done <"$manifest"

printf '%s: %d/%d passed\n' "$stage_name" "$passed" "$total"
exit "$has_failure"
