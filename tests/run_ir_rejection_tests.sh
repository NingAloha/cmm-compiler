#!/bin/sh

set -u

if [ "$#" -ne 2 ]; then
    printf '%s\n' "Usage: $0 <parser> <manifest>" >&2
    exit 2
fi

parser=$1
manifest=$2
stage_name=$(basename "$(dirname "$manifest")")
suite_name="$stage_name/rejections"
passed=0
total=0
has_failure=0

while IFS= read -r source_file; do
    case "$source_file" in
    '' | \#*)
        continue
        ;;
    esac

    total=$((total + 1))
    output_file=$(mktemp "${TMPDIR:-/tmp}/cmm-compiler-rejected-ir.XXXXXX") || exit 2
    diagnostic_output=$(mktemp "${TMPDIR:-/tmp}/cmm-compiler-rejection-diagnostic.XXXXXX") || exit 2
    rm -f "$output_file"

    "$parser" "$source_file" "$output_file" >"$diagnostic_output" 2>&1
    actual_status=$?

    if [ "$actual_status" -ne 0 ] && [ ! -e "$output_file" ]; then
        passed=$((passed + 1))
    else
        has_failure=1
        printf '%s: FAIL %s\n' "$suite_name" "$source_file"
        printf '%s\n' '  expected a nonzero exit status and no IR file'
        printf '  actual exit status: %s\n' "$actual_status"
        printf '  IR file exists: %s\n' "$(test -e "$output_file" && echo yes || echo no)"
        printf '%s\n' '  diagnostic output:'
        sed 's/^/    /' "$diagnostic_output"
    fi

    rm -f "$output_file" "$diagnostic_output"
done <"$manifest"

printf '%s: %d/%d passed\n' "$suite_name" "$passed" "$total"
exit "$has_failure"
