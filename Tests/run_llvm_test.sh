#!/usr/bin/env sh
# Builds and runs test packages using the LLVM backend
# Verifies that the LLVM backend produces working executables
#
# Usage:
#   Tests/run_llvm_test.sh [path-to-rux]
#   RUX=/path/to/rux Tests/run_llvm_test.sh
#
# The rux binary is taken from $RUX, then the first argument, then a few common
# build locations, then $PATH.
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

RUX="${RUX:-${1:-}}"
if [ -z "$RUX" ]; then
    for candidate in \
        "$SCRIPT_DIR/../build/rux" \
        "$SCRIPT_DIR/../build/clang/rux" \
        "$SCRIPT_DIR/../build/msvc/rux" \
        "$SCRIPT_DIR/../build/Release/rux" \
        "$(command -v rux 2>/dev/null || true)"; do
        if [ -n "$candidate" ] && [ -x "$candidate" ]; then
            RUX="$candidate"
            break
        fi
    done
fi
if [ -z "$RUX" ]; then
    echo "error: rux binary not found; set RUX or pass it as the first argument" >&2
    exit 2
fi
echo "Using rux: $RUX"

# Check if LLVM backend is enabled
echo "Checking if LLVM backend is enabled..."
if "$RUX" build --help 2>&1 | grep -q -- "--use-llvm"; then
    echo "LLVM backend flag found in rux"
else
    echo "error: LLVM backend not enabled in this rux build" >&2
    echo "Rebuild with -DUSE_LLVM_BACKEND=ON" >&2
    exit 2
fi

# Builds the package in $1 with LLVM backend and prints the path to its executable on stdout.
build_pkg_llvm() {
    pkg="$SCRIPT_DIR/$1"
    name="$2"
    ( cd "$pkg" && "$RUX" build --use-llvm >/dev/null )
    bin="$pkg/Bin/Debug/$name"
    [ -f "$bin" ] || bin="$bin.exe"
    if [ ! -f "$bin" ]; then
        echo "error: built executable not found at $bin" >&2
        exit 2
    fi
    printf '%s' "$bin"
}

failures=0

# 1. stdout test with LLVM: WriteFile + GetStdHandle.
echo "Testing LLVM backend with Tests/Io..."
IO_BIN=$(build_pkg_llvm "Io" "io_test")
IO_EXPECTED="Hello from a Rux binary via I/O thunks!"
set +e
IO_ACTUAL=$("$IO_BIN")
IO_CODE=$?
set -e
if [ "$IO_ACTUAL" = "$IO_EXPECTED" ] && [ "$IO_CODE" -eq 0 ]; then
    echo "PASS: stdout thunks with LLVM (Tests/Io)"
else
    echo "FAIL: stdout thunks with LLVM (Tests/Io)" >&2
    echo "  expected: [$IO_EXPECTED] (exit 0)" >&2
    echo "  actual:   [$IO_ACTUAL] (exit $IO_CODE)" >&2
    failures=$((failures + 1))
fi

# 2. stdin round-trip with LLVM: ReadFile + WriteFile + GetStdHandle.
echo "Testing LLVM backend with Tests/Echo..."
ECHO_BIN=$(build_pkg_llvm "Echo" "echo_test")
ECHO_INPUT="round-trip via stdin thunks"
set +e
ECHO_ACTUAL=$(printf '%s' "$ECHO_INPUT" | "$ECHO_BIN")
ECHO_CODE=$?
set -e
if [ "$ECHO_ACTUAL" = "$ECHO_INPUT" ] && [ "$ECHO_CODE" -eq 0 ]; then
    echo "PASS: stdin round-trip with LLVM (Tests/Echo)"
else
    echo "FAIL: stdin round-trip with LLVM (Tests/Echo)" >&2
    echo "  sent:     [$ECHO_INPUT]" >&2
    echo "  received: [$ECHO_ACTUAL] (exit $ECHO_CODE)" >&2
    failures=$((failures + 1))
fi

# 3. Test LLVM IR emission
echo "Testing LLVM IR emission..."
if ( cd "$SCRIPT_DIR/Io" && "$RUX" build --use-llvm --emit-llvm >/dev/null 2>&1 ); then
    if [ -f "$SCRIPT_DIR/Io/Temp/LLVM/out.ll" ]; then
        echo "PASS: LLVM IR emission (Tests/Io)"
    else
        echo "FAIL: LLVM IR file not generated" >&2
        failures=$((failures + 1))
    fi
else
    echo "FAIL: LLVM IR emission failed" >&2
    failures=$((failures + 1))
fi

if [ "$failures" -eq 0 ]; then
    echo "All LLVM backend tests passed"
    exit 0
fi
echo "$failures LLVM backend test(s) failed" >&2
exit 1
