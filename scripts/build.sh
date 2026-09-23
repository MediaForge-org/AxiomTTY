#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_NAME="AxiomTTY"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="${AXIOMTTY_BUILD_TYPE:-Debug}"
BUILD_DIR="${AXIOMTTY_BUILD_DIR:-$HOME/.cache/AxiomTTY-build}"
GENERATOR="${AXIOMTTY_GENERATOR:-Ninja}"
JOBS="${AXIOMTTY_JOBS:-$(nproc 2>/dev/null || echo 4)}"

RUN_AFTER_BUILD=0
RUN_TESTS=1
REBUILD=0
CLEAN_ONLY=0
CONFIGURE_ONLY=0
BUILD_ONLY=0
VERBOSE=0
LIST_TESTS=0
SHOW_INFO=0
TEST_REGEX=""
TARGET=""
EXTRA_CMAKE_ARGS=()
EXTRA_BUILD_ARGS=()
APP_ARGS=()

BOLD=$'\033[1m'
GREEN=$'\033[32m'
YELLOW=$'\033[33m'
RED=$'\033[31m'
CYAN=$'\033[36m'
RESET=$'\033[0m'

log() { printf '%s%s==>%s %s\n' "$BOLD" "$CYAN" "$RESET" "$*"; }
ok() { printf '%s%s✓%s %s\n' "$BOLD" "$GREEN" "$RESET" "$*"; }
warn() { printf '%s%s!%s %s\n' "$BOLD" "$YELLOW" "$RESET" "$*" >&2; }
die() { printf '%s%sERROR:%s %s\n' "$BOLD" "$RED" "$RESET" "$*" >&2; exit 1; }

usage() {
    cat <<USAGE
AxiomTTY build helper

Usage:
  bash scripts/build.sh [options] [-- app arguments...]

Main:
  --run                 Build/test and start AxiomTTY
  --debug               Use Debug build (default)
  --release             Use Release build
  --relwithdebinfo      Use RelWithDebInfo build
  --rebuild             Remove build dir before configuring/building
  --clean               Remove build dir and exit
  --configure-only      Configure CMake only
  --build-only          Skip configure and build existing build directory
  --no-tests            Do not run CTest
  --list-tests          List CTest tests after configure/build
  --test REGEX          Run only tests matching REGEX
  --target NAME         Build only a specific CMake target
  -j, --jobs N          Parallel build jobs (default: $JOBS)
  -v, --verbose         Verbose CMake build output
  --info                Show resolved build configuration and exit
  -h, --help            Show this help

Paths:
  --build-dir PATH      Override build directory
                        default: $BUILD_DIR
  --generator NAME      CMake generator (default: $GENERATOR)

Advanced:
  --cmake ARG           Pass one extra argument to CMake configure
                        Can be used multiple times
  --build-arg ARG       Pass one extra argument to cmake --build
                        Can be used multiple times

Examples:
  bash scripts/build.sh
  bash scripts/build.sh --run
  bash scripts/build.sh --rebuild --run
  bash scripts/build.sh --release --run
  bash scripts/build.sh --test settings_smoke
  bash scripts/build.sh --target axiomtty --no-tests
  bash scripts/build.sh --build-dir ~/tmp/axiom-build --run
USAGE
}

need_command() { command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"; }

print_info() {
    cat <<INFO
Project:       $PROJECT_NAME
Source:        $PROJECT_ROOT
Build dir:     $BUILD_DIR
Build type:    $BUILD_TYPE
Generator:     $GENERATOR
Jobs:          $JOBS
Tests:         $([[ "$RUN_TESTS" -eq 1 ]] && echo enabled || echo disabled)
Run after:     $([[ "$RUN_AFTER_BUILD" -eq 1 ]] && echo yes || echo no)
Target:        ${TARGET:-all}
Test regex:    ${TEST_REGEX:-all}
INFO
}

cleanup_on_error() {
    local rc=$?
    if [[ $rc -ne 0 ]]; then
        printf '\n'
        warn "Build helper stopped with exit code $rc."
        warn "Build directory was kept for inspection: $BUILD_DIR"
    fi
    exit "$rc"
}
trap cleanup_on_error ERR

while [[ $# -gt 0 ]]; do
    case "$1" in
        --run) RUN_AFTER_BUILD=1; shift ;;
        --debug) BUILD_TYPE="Debug"; shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --relwithdebinfo) BUILD_TYPE="RelWithDebInfo"; shift ;;
        --rebuild) REBUILD=1; shift ;;
        --clean) CLEAN_ONLY=1; shift ;;
        --configure-only) CONFIGURE_ONLY=1; shift ;;
        --build-only) BUILD_ONLY=1; shift ;;
        --no-tests) RUN_TESTS=0; shift ;;
        --list-tests) LIST_TESTS=1; shift ;;
        --test) [[ $# -ge 2 ]] || die "--test requires a regex"; TEST_REGEX="$2"; shift 2 ;;
        --target) [[ $# -ge 2 ]] || die "--target requires a target name"; TARGET="$2"; shift 2 ;;
        -j|--jobs) [[ $# -ge 2 ]] || die "$1 requires a number"; JOBS="$2"; [[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || die "Invalid jobs value: $JOBS"; shift 2 ;;
        -v|--verbose) VERBOSE=1; shift ;;
        --info) SHOW_INFO=1; shift ;;
        --build-dir) [[ $# -ge 2 ]] || die "--build-dir requires a path"; BUILD_DIR="$2"; shift 2 ;;
        --generator) [[ $# -ge 2 ]] || die "--generator requires a name"; GENERATOR="$2"; shift 2 ;;
        --cmake) [[ $# -ge 2 ]] || die "--cmake requires an argument"; EXTRA_CMAKE_ARGS+=("$2"); shift 2 ;;
        --build-arg) [[ $# -ge 2 ]] || die "--build-arg requires an argument"; EXTRA_BUILD_ARGS+=("$2"); shift 2 ;;
        -h|--help) usage; exit 0 ;;
        --) shift; APP_ARGS=("$@"); break ;;
        *) die "Unknown option: $1 (use --help)" ;;
    esac
done

need_command cmake
need_command ctest
if [[ "$GENERATOR" == "Ninja" ]]; then need_command ninja; fi
[[ -f "$PROJECT_ROOT/CMakeLists.txt" ]] || die "CMakeLists.txt not found at $PROJECT_ROOT"

if [[ "$SHOW_INFO" -eq 1 ]]; then print_info; exit 0; fi

if [[ "$CLEAN_ONLY" -eq 1 ]]; then
    log "Cleaning build directory"
    rm -rf -- "$BUILD_DIR"
    ok "Removed $BUILD_DIR"
    exit 0
fi

if [[ "$REBUILD" -eq 1 ]]; then
    log "Removing previous build directory"
    rm -rf -- "$BUILD_DIR"
fi

mkdir -p -- "$BUILD_DIR"

printf '%s\n' "------------------------------------------------------------"
print_info
printf '%s\n' "------------------------------------------------------------"

if [[ "$BUILD_ONLY" -eq 0 ]]; then
    log "Configuring CMake"
    CMAKE_CONFIGURE_ARGS=(-S "$PROJECT_ROOT" -B "$BUILD_DIR" -G "$GENERATOR" "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    if command -v ccache >/dev/null 2>&1; then
        CMAKE_CONFIGURE_ARGS+=("-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
        ok "ccache detected"
    else
        warn "ccache not found; building without compiler cache"
    fi
    CMAKE_CONFIGURE_ARGS+=("${EXTRA_CMAKE_ARGS[@]}")
    cmake "${CMAKE_CONFIGURE_ARGS[@]}"
    ok "Configure complete"
fi

if [[ "$CONFIGURE_ONLY" -eq 1 ]]; then exit 0; fi

log "Building"
BUILD_CMD=(cmake --build "$BUILD_DIR" --parallel "$JOBS")
[[ -n "$TARGET" ]] && BUILD_CMD+=(--target "$TARGET")
[[ "$VERBOSE" -eq 1 ]] && BUILD_CMD+=(--verbose)
BUILD_CMD+=("${EXTRA_BUILD_ARGS[@]}")
"${BUILD_CMD[@]}"
ok "Build complete"

if [[ "$LIST_TESTS" -eq 1 ]]; then
    log "Available tests"
    ctest --test-dir "$BUILD_DIR" -N
fi

if [[ "$RUN_TESTS" -eq 1 ]]; then
    log "Running tests"
    TEST_CMD=(ctest --test-dir "$BUILD_DIR" --output-on-failure --parallel "$JOBS")
    [[ -n "$TEST_REGEX" ]] && TEST_CMD+=(-R "$TEST_REGEX")
    "${TEST_CMD[@]}"
    ok "Tests passed"
fi

BINARY="$BUILD_DIR/axiomtty"

if [[ "$RUN_AFTER_BUILD" -eq 1 ]]; then
    [[ -x "$BINARY" ]] || die "AxiomTTY binary not found or not executable: $BINARY"
    log "Starting AxiomTTY"
    printf '\n'
    exec "$BINARY" "${APP_ARGS[@]}"
fi

printf '\n'
ok "AxiomTTY build finished"
printf 'Run with:\n  %s\n' "$BINARY"
printf 'Or build + run next time with:\n  bash scripts/build.sh --run\n'
