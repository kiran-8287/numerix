# Phase 14C benchmark contract (draft v1)

This contract applies to new measurements under
`benchmarks/results/phase14c/module_comparisons/`. It does not relabel or
rewrite Phase 14 or Phase 14B artifacts. Reused historical data must retain its
original `run_id`, source path, checksum and dependency versions.

## Required provenance and record families

Every run records its immutable run ID, UTC start/end, Git head and dirty state,
OS/kernel/libc, CPU model and core count, compiler/version/flags, dependency
versions/build options, thread limits, workload-manifest checksum, adapter
checksum, timer, random seed, and invocation. The fixed primary competitors are
GSL 2.8 (root, differentiation, interpolation, integration), LAPACK with
OpenBLAS (linear algebra), NLopt 2.10.1 (optimization), and SUNDIALS 7.9.0
(ODE). A local version mismatch is a blocker to a new measurement under that
mapping; it must not be silently substituted.

`workload_manifest.csv` is frozen and checksummed before final measurements.
Each row has a unique problem ID, module/family, mathematical definition,
serialized parameters, problem size, applicable public methods, reference
value/method/precision, target absolute and relative accuracy, and a
comparison-equivalence classification. Inputs are generated once and passed
identically to both implementations. Failed launches and method failures stay
in the output population.

The stable workload columns are `module,problem_id,family,definition,
parameters_json,input_data_path,input_data_sha256,input_size,difficulty,
applicable_methods_json,reference_value_json,reference_method,
reference_precision_digits,tolerance_abs,tolerance_rel,comparison_type,
source_workload_id,source_run_id`. If a method cannot apply to an input, the
manifest records an explicit reason or a separate applicability table does;
the method is not dropped after observing its result.

`raw_timing.csv` has one row per measured timing batch and at least: run ID,
module, problem ID, library and exact version, NUMERIX method, competitor
method, direct/best-available classification, batch index/order, batch size,
warmup count, monotonic wall elapsed seconds, CPU elapsed seconds, status and
native status, and work counters when available. Each route is warmed up and
measured in 31 batches where the pilot establishes stable timer resolution.
Library order is randomized/counterbalanced by problem and batch. Setup and
steady-state calls are separate; an end-to-end timing is retained when setup
is part of the normal public operation. The per-problem median, not the batch,
is the primary statistical unit.

The stable timing columns are `run_id,module,problem_id,library,
library_version,numerix_method,competitor_method,comparison_type,
timing_scope,batch_index,call_order,order_seed,batch_size,warmup_count,
wall_elapsed_seconds,wall_seconds_per_call,cpu_elapsed_seconds,
cpu_seconds_per_call,normalized_status,native_status,converged,
accuracy_pass,work_metrics_json`.

`raw_memory.csv` records one isolated process/sample per operation and includes
peak additional live requested bytes, total requested bytes, allocation,
reallocation and free counts, instrumentation method, and setup/operation
scope. Input/harness buffers are allocated before the measurement scope and
output allocations are handled consistently and labeled. Missing allocator
visibility is `unavailable` with a reason, never zero. RSS is retained only as
process-level context and cannot be called method-specific memory. Allocation
instrumentation is validated against known allocations before relying on it.

The stable memory columns are `run_id,module,problem_id,library,
library_version,numerix_method,competitor_method,comparison_type,
measurement_scope,memory_status,memory_unavailable_reason,
baseline_live_requested_bytes,peak_additional_live_requested_bytes,
total_requested_bytes,allocation_count,reallocation_count,free_count,
peak_process_rss_bytes,instrumentation,validation_id`.

`raw_accuracy.csv` records every attempted problem/method/library, native and
normalized status, finite-result flag, reference and reference provenance,
metric definition, absolute/relative/normalized error as applicable, target,
pass flag, and work counters. References are independent of both tested
outputs. Failed/nonfinite cases remain present and do not disappear from
coverage counts.

The stable accuracy columns are `run_id,module,problem_id,library,
library_version,numerix_method,competitor_method,comparison_type,
normalized_status,native_status,converged,is_finite,accuracy_metric,
reference_value_json,observed_value_json,absolute_error,relative_error,
normalized_error,tolerance_abs,tolerance_rel,accuracy_pass,work_metrics_json`.

## Analysis and figure rules

Runtime summaries use per-problem batch medians and paired competitor/NUMERIX
ratios. Bootstrap resampling samples problems, not batches. Every summary keeps
all-attempt coverage alongside successful and accuracy-qualified coverage.
Accuracy uses a module-appropriate normalized metric and retains failed cases.
Memory is graphed only where allocator scope and visibility passed verification;
otherwise it is explicitly unavailable. A method-family difference is labeled
best available, never direct.

The deliverable allows exactly three figure types per module: runtime, memory,
and accuracy. Each is generated from saved CSV/JSON by a single versioned
plotting script and saved as SVG and PNG. If a metric has no defensible sample,
the figure must state “not measured” and the report must mark the module
incomplete; no placeholder is described as benchmark evidence. No speedup,
scalability, dashboard, or other fourth plot type is generated.

## Verification gates

Before a full run: verify unique/frozen workloads, actual API dispatch,
independent reference checks, adapter input parity, timer calibration,
allocation-wrapper behavior, and compiler/dependency versions in a pilot.
Before graphing: reconcile attempted record counts against the workload and
method manifests; verify 31 batch counts (where feasible), no negative/invalid
times, all process exits, raw-to-summary reconstruction, and memory/accuracy
scope. Each graph must be reproducible from this run's processed data. Historical
Phase 14/14B records are comparative context only and are not new Phase 14C
measurements.

## Environment and dependency status

System paths report GSL 2.5, OpenBLAS 0.3.15 and LAPACK/LAPACKE 3.8.0; those
system copies are not selected for the fixed-version measurements. Isolated
Phase 14 dependency prefixes were found and verified: GSL 2.8 at
`/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`, OpenBLAS 0.3.34 at
`/home/142401034/.local/numerix-deps/prefix/phase14/openblas`, and SUNDIALS
7.9.0 at `/home/142401034/.local/numerix-deps/prefix/phase14/sundials`. NLopt
2.10.1 was built from its `v2.10.1` source archive into the separate prefix
`/home/142401034/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1`; source
archive SHA-256 is
`30d13ce16da119db3e987784f7864e35a562ec62c186352fae55cd003e6c58ff`. Existing
NLopt 2.11.0 remains intact. Reference LAPACK metadata under the older Phase 14
prefix reports 3.12.0, although that phase's library manifest says 3.12.1;
Phase 14C will label the exact selected backend version and preserve this
historical discrepancy rather than rewriting it.

These checks establish that required library versions can be loaded from
isolated prefixes. The allocation wrapper's known-control, GSL 2.8 vector, and
NUMERIX vector-allocation probes also pass on this Linux/glibc host. The wrapper
does not observe direct `mmap` or custom allocators, and route-level visibility
for OpenBLAS/LAPACK, NLopt, and SUNDIALS remains unverified. The current adapter
runs are small pilots only; the full method set, 100-problem-per-module study,
and method-specific memory series have not passed their gates.
