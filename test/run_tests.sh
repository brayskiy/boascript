#!/usr/bin/env bash
#
# BoaScript golden-file test harness, modeled on the ooyacc test suite.
#
# For every program in cases/<name>.boa:
#   1. run it through the driver (which calls Boascript::run)
#   2. diff its stdout against the golden file cases/<name>.expected
#
# Exit status is non-zero if any case fails.

set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER="$HERE/driver"
CASES="$HERE/cases"
BUILD="$HERE/build"

if [ ! -x "$DRIVER" ]; then
    echo "Building driver ..."
    make -C "$HERE" driver >/dev/null || { echo "driver build failed"; exit 1; }
fi

mkdir -p "$BUILD"
pass=0
fail=0
failed=()

for boa in "$CASES"/*.boa; do
    name="$(basename "$boa" .boa)"
    expected="$CASES/$name.expected"

    if [ ! -f "$expected" ]; then
        echo "[NO GOLDEN]    $name"
        fail=$((fail+1)); failed+=("$name"); continue
    fi

    "$DRIVER" "$boa" > "$BUILD/$name.actual" 2> "$BUILD/$name.stderr"

    if diff -u "$expected" "$BUILD/$name.actual" > "$BUILD/$name.diff"; then
        echo "[PASS]         $name"
        pass=$((pass+1))
    else
        echo "[FAIL]         $name"; sed 's/^/    /' "$BUILD/$name.diff"
        fail=$((fail+1)); failed+=("$name")
    fi
done

echo "--------------------------------------------------"
echo "Passed: $pass   Failed: $fail"
if [ "$fail" -ne 0 ]; then
    echo "Failing: ${failed[*]}"
    exit 1
fi
