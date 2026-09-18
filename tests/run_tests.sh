#!/bin/sh

set -u

if [ "$#" -ne 2 ]; then
    printf '%s\n' "Usage: $0 <parser> <manifest>" >&2
    exit 2
fi

parser=$1
manifest=$2
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

    "$parser" "$test_file" >"$output_file" 2>&1
    actual_status=$?

    if [ "$actual_status" -eq "$expected_status" ] \
        && grep -Eq "$expected_pattern" "$output_file"; then
        passed=$((passed + 1))
    else
        has_failure=1
        printf '[FAIL] %s\n' "$test_file"
        cat "$output_file"
    fi

    rm -f "$output_file"
done < "$manifest"

printf 'Passed: %d/%d\n' "$passed" "$total"
exit "$has_failure"
