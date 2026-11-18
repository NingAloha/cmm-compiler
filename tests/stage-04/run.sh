#!/bin/sh
set -eu
export LC_ALL=C

if [ "$#" -ne 2 ]; then
  printf 'Usage: %s /path/to/cc /path/to/spim\n' "$0" >&2
  exit 2
fi

# Keep executable paths relative to the caller, not to this script.
case $1 in /*) compiler=$1 ;; *) compiler=./$1 ;; esac
case $2 in
/*) spim=$2 ;;
*/*) spim=./$2 ;;
*) spim=$(command -v "$2") || {
  printf 'SPIM not found: %s\n' "$2" >&2
  exit 2
} ;;
esac
for executable in "$compiler" "$spim"; do
  if [ ! -f "$executable" ] || [ ! -x "$executable" ]; then
    printf 'Not an executable file: %s\n' "$executable" >&2
    exit 2
  fi
done

suite=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/cmm-stage04.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
trap 'exit 1' HUP INT TERM

run_case() {
  source=$1
  stem=${source%.cmm}
  expected=$stem.out
  input=/dev/null
  if [ -f "$stem.in" ]; then input=$stem.in; fi
  if [ ! -f "$expected" ]; then
    printf 'Missing expected output: %s\n' "$expected" >&2
    return 1
  fi

  assembly=$work/${stem##*/}.s
  if "$compiler" "$source" "$assembly" >"$work/cc.log" 2>&1; then
    if [ ! -s "$assembly" ]; then
      printf 'Compiler did not generate nonempty assembly\n' >&2
      cat "$work/cc.log" >&2
      return 1
    fi
  else
    printf 'Compiler exited with status %s\n' "$?" >&2
    cat "$work/cc.log" >&2
    return 1
  fi

  if "$spim" -file "$assembly" <"$input" >"$work/spim.out" 2>"$work/spim.err"; then
    :
  else
    printf 'SPIM exited with status %s\n' "$?" >&2
    cat "$work/spim.out" "$work/spim.err" >&2
    return 1
  fi
  if [ -s "$work/spim.err" ]; then
    printf 'SPIM wrote diagnostics to stderr\n' >&2
    cat "$work/spim.out" "$work/spim.err" >&2
    return 1
  fi

  # Strip only known startup lines and the runtime's read prompt. In
  # particular, leave all unexpected stdout (including diagnostics) intact.
  if ! sed -e '1,5{
        /^SPIM Version /d
        /^Copyright .*James R\. Larus/d
        /^All Rights Reserved\.$/d
        /^See the file README for a full copyright notice\.$/d
        /^Loaded: .*\/exceptions\.s$/d
    }' -e 's/^\(Enter an integer:\( \)\{0,1\}\)*//' \
    "$work/spim.out" >"$work/actual"; then
    return 1
  fi
  diff -u "$expected" "$work/actual"
}

passed=0
failed=0
for source in "$suite"/cases/*.cmm; do
  if [ ! -f "$source" ]; then
    printf 'No Stage04 cases found in %s/cases\n' "$suite" >&2
    exit 2
  fi
  if run_case "$source"; then
    printf 'PASS %s\n' "${source##*/}"
    passed=$((passed + 1))
  else
    printf 'FAIL %s\n' "${source##*/}" >&2
    failed=$((failed + 1))
  fi
done
printf 'Stage04: %s passed, %s failed\n' "$passed" "$failed"
[ "$failed" -eq 0 ]
