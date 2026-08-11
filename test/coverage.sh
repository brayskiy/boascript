#!/usr/bin/env bash
#
# Measure line and branch coverage of the interpreter (src/Boascript.y,
# reached through the generated src/Boascript.tab.cpp) exercised by both
# test suites: the embedded assertion table (testboascript) and the
# golden-file cases (driver over cases/*.boa), plus the focused unit tests
# in unittests.cpp.
#
# Requires gcov (from gcc). Run from the repo root or the test directory.

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST="$ROOT/test"
COV="$TEST/coverage"
CXX="${CXX:-g++}"
COVFLAGS="--coverage -O0 -g -DCALC_BATCH -I$ROOT/distribution -I$ROOT/extras"

# The generated parser must exist (make all from the repo root builds it).
if [ ! -f "$ROOT/src/Boascript.tab.cpp" ]; then
    echo "src/Boascript.tab.cpp missing -- run 'make all' from the repo root first."
    exit 1
fi

rm -rf "$COV"; mkdir -p "$COV"

# Compile the instrumented objects from the repo root so the relative
# "#line src/Boascript.y" paths resolve.
( cd "$ROOT" && \
  $CXX $COVFLAGS -c src/Boascript.tab.cpp -o "$COV/Boascript.tab.o" && \
  $CXX $COVFLAGS -c extras/DateTime.cpp   -o "$COV/DateTime.o" && \
  $CXX $COVFLAGS -c test/testboascript.cpp -o "$COV/testboascript.o" && \
  $CXX $COVFLAGS -c test/driver.cpp        -o "$COV/driver.o" && \
  $CXX $COVFLAGS -c test/unittests.cpp     -o "$COV/unittests.o" ) || exit 1

$CXX --coverage -o "$COV/run_embedded" "$COV/testboascript.o" "$COV/Boascript.tab.o" "$COV/DateTime.o"
$CXX --coverage -o "$COV/run_driver"   "$COV/driver.o"        "$COV/Boascript.tab.o" "$COV/DateTime.o"
$CXX --coverage -o "$COV/run_unit"     "$COV/unittests.o"     "$COV/Boascript.tab.o" "$COV/DateTime.o"

# Run every suite; coverage counters accumulate into the shared .gcda.
"$COV/run_embedded" > /dev/null 2>&1
"$COV/run_unit"     > /dev/null 2>&1
for boa in "$TEST"/cases/*.boa; do
    "$COV/run_driver" "$boa" > /dev/null 2>&1
done

# Summarize coverage for the interpreter source.
( cd "$ROOT" && gcov -b -o "$COV" src/Boascript.tab.cpp > "$COV/gcov.log" 2>&1 )
mv "$ROOT"/*.gcov "$COV"/ 2>/dev/null

echo "=================== Coverage: src/Boascript.y ==================="
awk '
    /File .*Boascript\.y/      { infile = 1 }
    infile && /Lines executed/ { print "  " $0 }
    infile && /^$/             { infile = 0 }
' "$COV/gcov.log"

# Branch coverage, computed two ways from the annotated .gcov. gcov counts
# every implicit exception-cleanup edge as a branch; coverage tools (lcov's
# geninfo_no_exception_branches) exclude those by default because they are
# compiler-generated, not program logic. We report both.
awk '
    /branch +[0-9]+ / {
        is_throw = ($0 ~ /\(throw\)/)
        taken = ($0 ~ /taken [1-9]/)
        rtotal++; if (taken) rtook++
        if (!is_throw) { total++; if (taken) took++ }
    }
    END {
        printf "  Branches (raw, incl. exception edges): %.2f%% of %d\n", 100.0*rtook/rtotal, rtotal
        printf "  Branches (excl. exception edges):      %.2f%% of %d\n", 100.0*took/total, total
    }
' "$COV/Boascript.y.gcov"
echo "================================================================"
echo "Detail: $COV/Boascript.y.gcov   (log: $COV/gcov.log)"
