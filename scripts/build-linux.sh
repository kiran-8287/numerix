#!/usr/bin/env bash
set -euo pipefail

configuration=Release
compiler=gcc
run_examples=0

usage() {
    cat <<'EOF'
Usage: scripts/build-linux.sh [--config Debug|Release] [--compiler gcc|clang] [--run-examples]

Configures a separate build/linux-<compiler>-<configuration> directory,
builds all default targets, and runs the registered CTest suite.
EOF
}

while (($#)); do
    case "$1" in
        --config)
            (($# >= 2)) || { usage >&2; exit 2; }
            configuration=$2
            shift 2
            ;;
        --compiler)
            (($# >= 2)) || { usage >&2; exit 2; }
            compiler=$2
            shift 2
            ;;
        --run-examples)
            run_examples=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf 'Unknown argument: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

case "$configuration" in
    Debug|Release) ;;
    *) printf 'Configuration must be Debug or Release.\n' >&2; exit 2 ;;
esac
case "$compiler" in
    gcc)
        c_compiler=gcc
        cxx_compiler=g++
        ;;
    clang)
        c_compiler=clang
        cxx_compiler=clang++
        ;;
    *) printf 'Compiler must be gcc or clang.\n' >&2; exit 2 ;;
esac

command -v cmake >/dev/null || { echo 'CMake was not found on PATH.' >&2; exit 127; }
command -v "$c_compiler" >/dev/null || { echo "$c_compiler was not found on PATH." >&2; exit 127; }

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$script_dir/.." && pwd)
build_dir="$repo_root/build/linux-${compiler}-$(printf '%s' "$configuration" | tr '[:upper:]' '[:lower:]')"

printf 'Configuring %s %s in %s\n' "$compiler" "$configuration" "$build_dir"
cmake -S "$repo_root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$configuration" \
    -DCMAKE_C_COMPILER="$c_compiler" \
    -DCMAKE_C_FLAGS=-Werror
cmake --build "$build_dir" --parallel
cmake -E chdir "$build_dir" ctest --output-on-failure

install_dir="$build_dir/installed"
consumer_dir="$build_dir/installed-consumer"
cmake --install "$build_dir" --prefix "$install_dir"
cmake -S "$repo_root/tests/installed_consumer" -B "$consumer_dir" \
    -DNUMERIX_INSTALL_PREFIX="$install_dir" \
    -DCMAKE_BUILD_TYPE="$configuration" \
    -DCMAKE_C_COMPILER="$c_compiler" \
    -DCMAKE_CXX_COMPILER="$cxx_compiler" \
    -DCMAKE_C_FLAGS=-Werror -DCMAKE_CXX_FLAGS=-Werror
cmake --build "$consumer_dir" --parallel
cmake -E chdir "$consumer_dir" ctest --output-on-failure

if ((run_examples)); then
    cli="$build_dir/numerix_cli"
    "$cli" --help
    "$cli" --eval 'sin(pi/2)+x^2' 2
    "$cli" --demo
    "$cli" --root 'x^2-2' 0 2
    "$cli" --roots '(x-1)*(x-2)' 0 3
    "$cli" --piecewise-demo

    for demo in expression piecewise intelligence managed_execution root_discovery; do
        "$build_dir/numerix_${demo}_demo"
    done
fi

printf 'NUMERIX %s build and CTest suite succeeded (%s).\n' "$configuration" "$compiler"
