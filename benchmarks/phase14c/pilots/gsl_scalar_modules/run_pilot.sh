#!/usr/bin/env bash
set -euo pipefail
export OMP_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1

run_id=${1:-phase14c-pilot-scalar-$(date -u +'%Y%m%dT%H%M%SZ')}
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../../.." && pwd)
gsl_prefix=/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean
results_root="$repo_root/benchmarks/results/phase14c/module_comparisons"
pilot_dir="$repo_root/benchmarks/phase14c/pilots/gsl_scalar_modules"
pilot_bin="/tmp/numerix-$run_id"
start_utc=$(date -u +'%Y-%m-%dT%H:%M:%SZ')
return_code=0

cd "$repo_root" || exit 2
for module in differentiation interpolation integration; do
    mkdir -p "$results_root/raw/$module/$run_id"
done
python3 "$pilot_dir/write_support.py" "$run_id" prepare || exit 2
manifest_path="$results_root/manifests/pilot_workload_$run_id.csv"
sha256sum "$manifest_path" > "$results_root/manifests/pilot_workload_checksum_$run_id.txt"

gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -Iinclude -I"$gsl_prefix/include" \
    "$pilot_dir/pilot.c" build/linux-gcc-release/libnumerix.a \
    -L"$gsl_prefix/lib" -Wl,-rpath,"$gsl_prefix/lib" \
    -lgsl -lgslcblas -lm -o "$pilot_bin" || exit 2

ldd "$pilot_bin" | rg 'libgsl\.so|libgslcblas\.so' > "$results_root/manifests/gsl_linkage_$run_id.txt" || exit 2
if ! rg -q "$gsl_prefix/lib" "$results_root/manifests/gsl_linkage_$run_id.txt"; then
    echo "Pilot did not resolve GSL from the requested 2.8 prefix." >&2
    exit 3
fi

if env LD_LIBRARY_PATH="$gsl_prefix/lib" OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 \
    "$pilot_bin" "$run_id" "$results_root"; then
    return_code=0
else
    return_code=$?
fi

python3 "$pilot_dir/write_support.py" "$run_id" finalize \
    --start-utc "$start_utc" --return-code "$return_code"
python3 "$pilot_dir/verify_pilot.py" "$run_id"
exit "$return_code"
