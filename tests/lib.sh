#!/bin/sh
# Shared helpers for the manifest-driven test runners (source this file).

# make_temp <label>: create a temp file and print its path.
make_temp() {
    mktemp "${TMPDIR:-/tmp}/cmm-compiler-$1.XXXXXX" || exit 2
}

# is_skipped_line <line>: true for blank lines and '#' comments.
is_skipped_line() {
    case "$1" in
    '' | \#*) return 0 ;;
    esac
    return 1
}

# print_indented <file>: print a file indented by four spaces.
print_indented() {
    sed 's/^/    /' "$1"
}

# init_counters <suite>: initialise the suite name and counters.
init_counters() {
    suite_name=$1
    passed=0
    total=0
    has_failure=0
}

# finish_suite: print the summary and exit with the suite status.
finish_suite() {
    printf '%s: %d/%d passed\n' "$suite_name" "$passed" "$total"
    exit "$has_failure"
}

# suite_name_for <manifest> <kind>: e.g. stage-03/cases.
suite_name_for() {
    printf '%s/%s' "$(basename "$(dirname "$1")")" "$2"
}
