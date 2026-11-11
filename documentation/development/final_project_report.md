# NUMERIX final project report

Date: 2026-11-12. Candidate: **0.2.0-rc1**. Final assessment:
**release candidate with documented limitations**.

## A. Project overview

NUMERIX is a C11 static numerical library with one public header and no mandatory
external numerical dependency. Phases 10–12 followed Phase 9 sequentially: numerical
hardening, available compiler/platform validation, then audit and distribution.
Each gate was documented and committed before the next phase began. The Phase 9
commit was preserved and its earlier authentication problem resolved; subsequent
work uses ordinary authenticated fast-forward pushes, never force pushes.

## B. Final architecture

The same 25 production sources implement core/platform services, seven numerical
categories, expressions, piecewise functions, intelligence, managed execution,
diagnostics and discovery. Make and CMake source lists were mechanically compared
with the source tree and match exactly. No duplicate/unused production implementation
or obsolete file requiring deletion was identified. Historical reports are preserved
in development/, separately from current user/API/architecture guides.

Core/options/status/allocation contracts remain shared; matrices/datasets/vector
ODEs retain specialized representations. Higher-level workflows dispatch existing
algorithms, own their outputs and retain borrowed inputs for documented lifetimes.
There is no architectural rewrite or general parallel-execution mechanism.

## C. Numerical capabilities

Roots include Bisection, safeguarded chord/false-position, Brent-style interpolation,
Newton, safeguarded Newton and secant. Dense operations include partial-pivoted LU,
Gaussian and Gauss-Jordan solves. Finite differences, polynomial interpolation,
composite/adaptive quadrature, golden-section/gradient minimization and fixed-step
Euler/Heun/RK4 ODEs are retained. No major algorithm family was added in this roadmap.

## D. Intelligent execution and diagnostics

Compiled immutable expressions, piecewise segments/boundary evidence, deterministic
recommendations, explicit overrides, root fallback/portfolios, managed cancellation,
progress/summaries and explicit Bisection pause/resume work together. Diagnostics
report residuals, estimates, conditioning/sensitivity and convergence evidence.
Unavailable metrics/thresholds use availability flags and NAN where applicable;
root position tolerances no longer invent residual-unit thresholds. Estimates and
heuristics are never presented as measured forward-error guarantees.

## E. Multiple-root discovery

Discovery preserves geometric result allocation and bounded direct workspace.
Refinement position tolerance is separate from its explicitly configured residual
threshold. Sample-only/merged roots no longer report residual magnitudes as
position errors. Existing sign-changing, tangential, repeated, close, endpoint,
piecewise, pole, unresolved-region and cancellation tests pass. Discovery and
multiplicity estimates remain incomplete numerical evidence, not proofs.

## F. Numerical accuracy findings

See [Phase 10](phase10_accuracy_report.md) and
[raw accuracy output](../../benchmarks/results/phase10/accuracy.txt).
The 66 root-method/problem pairs cover scales 1e-200..1e200, flat cubic/ninth-order
roots, exponentials, endpoints, limits and stagnation. Root methods no longer stop
on a small nonzero residual. Bracket enclosures and observed open-method step
contraction supply method-specific position evidence. Exact computed zeros remain
subject to callback underflow/cancellation limitations.

| Phase 9 issue | Current result under the same requested tolerances |
| --- | --- |
| Flat cubic Brent error 4.13e-4 | 5.27e-12 |
| Flat cubic Newton error 3.38e-4 | 1.03e-10, within its 1.3e-10 combined position tolerance |
| Small-scaled quadratic error 0.414 | Brent 7.68e-14; Newton at the rounded reference |
| sqrt endpoint adaptive failure | Actual error 4.18e-13, estimate about 1e-10, 829 evaluations, same depth 32 |

Adaptive Simpson uses global max-error refinement and a conservative successive-rule
error estimate/roundoff floor, retaining the Simpson/Richardson numerical rule.
It now uses bounded heap storage instead of recursive equal local budgets.
Depth/split/evaluation exhaustion, stagnation, roundoff inability and invalid
samples are explicit statuses. Improper endpoints are not silently transformed.

## G. Benchmark results

GSL 2.8 remains optional. Phase 9 raw data is unchanged; Phase 10's 222 observations
are the updated mathematical baseline. GSL's driver was updated to position/step
evidence too, so changes are not advertised as pure speed optimization. Repeated
batch statistics, analytical references, counters, accuracy/status misses and
comparison eligibility remain machine-readable.

All successful valid root benchmark rows now meet the position check. Ten
nonzero statuses remain (invalid brackets/Hilbert rejection), plus 13 successful-call
accuracy misses in ill-conditioned systems/fixed quadrature. These are recorded,
not removed. GSL adaptive quadrature/interpolation can be substantially more efficient;
NUMERIX has economical scalar and selected LU/minimization API costs. Different
algorithms/work, including RK4 step doubling, are explicitly unranked. No universal
winner or cross-platform performance claim is made.

## H. Memory and performance

All 222 linked-static allocation probes clean up to zero tracked live requested
bytes. Direct empty discovery peaks 544/1056/2080 B at 16/32/64 panels remain separate
from nested allocations, allocator overhead and process RSS. The broader empty
search peaks 904/1416/2440 B. One-root retained arrays are 30,144 B on the measured
Linux ABI. Adaptive Simpson adds a measured dynamic panel heap for robust global
budgeting; this reliability tradeoff is not hidden as an allocation-free operation.
Fault injection covers selected expression/discovery/integration allocation and
growth positions. It is not exhaustive whole-library failure coverage.

## I. Platform/compiler validation

See [Phase 11 matrix](phase11_portability_report.md) and
[final command logs](../../benchmarks/results/phase12/validation/results.json).
Linux x86_64 GCC 8.5/11.5/15.2 and Clang 21.1 Debug/Release builds passed. GCC 11
required its matching binutils; the initial mismatch and resolution are retained.
CMake 3.16.8 and 4.4.4 were actually exercised, including minimum-version package
consumption. Standard CTest has 33 tests, with 34 when allocation faults are enabled.
ASan/UBSan/leak detection and Valgrind checks passed; GCC/Clang analyzers and
exhaustive Cppcheck passed. Earlier five test-harness warnings were fixed.

Native Windows MinGW/MSVC and macOS CI was configured/pushed, but hosted runs
returned queued/empty-job states rather than execution results. Those platforms
remain **unvalidated**, with no invented compiler/test success. MSVC's C11 atomics
requirement is feature-checked, not assumed. No binaries for these platforms ship.

## J. API and ABI policy

All 126 public function declarations have static archive definitions. No public
signature/struct layout changed in Phases 10–12; tolerance/status semantics changed
intentionally for correctness. API/source compatibility differs from binary ABI
compatibility. The 0.x policy permits incompatible minor releases with rebuild
notes; CMake version matching stays within the same minor. 0.2.0-rc1 is numeric
package version 0.2.0, explicitly a candidate. No stable/shared ABI is promised.
Old precompiled clients must rebuild against matching headers/archive; do not mix
incompatible compiler/CRT/architecture builds. See the user compatibility guide.

## K. Installation and packaging

CMake installs a static archive, public header, license/changelog, relocatable
NUMERIX::numerix package/version files, pkg-config metadata and user/API/architecture
documentation. GSL is absent from production symbols and undefined references.
Make supports basic PREFIX/DESTDIR archive/header/docs installation; use CMake
for exported package metadata. Shared builds are explicitly rejected.

A completely separate copied CMake project consumes only the installed package,
testing C/C++ Debug/Release roots, LU, expression evaluation, error returns and
cleanup. It passed after moving the prefix. Relocated pkg-config consumption,
old-minor rejection and unsupported-shared rejection were exercised.

The curated source TGZ has a manifest, normalized metadata and SHA256 sidecar.
Two builds and a Git-free manifest rebuild produce identical bytes. It excludes
local absolute workspace/tool paths, build objects, IDE state, credentials, raw
environment logs and development-only reports. Packaged local documentation links
were checked. Its extraction built/tested with both Make and CMake in Debug and
Release. The validated Linux static binary TGZ was separately extracted and used
by real C/C++ consumers. Artifacts are prepared locally under dist/; hashes and
sizes are in [artifacts.json](../../benchmarks/results/phase12/artifacts.json).
No stable tag or GitHub release/binary publication was performed.

## L. Known limitations

Open-root/finite-sample/error estimates are heuristic; callbacks can underflow,
cancel, be discontinuous or supply wrong derivatives. Closed endpoint quadrature
rejects infinite samples and can miss unsampled oscillations. Fixed grids do not
supply adaptive error guarantees; ill-conditioned solves can have large forward
error despite tiny backward error. General stepwise execution, shared libraries,
a custom allocator and a stable ABI remain absent. Native hosted-platform results
are still missing. Tests and static analysis are evidence, not proofs.

## M. Future recommendations

Obtain actual Windows/MSVC/macOS results; broaden adversarial/scaling/noisy-function
and hardware coverage; extend allocation-failure and fuzz testing; review the new
root contract with consumers; and define a deliberate ABI strategy before 1.x.
Keep numerical evidence qualified, preserve raw failures, and require benchmarks
and regression tests for subsequent accuracy/performance changes.

## N. Final assessment

**Release candidate with documented limitations.** The source/static candidate is
buildable, installable, relocatable, independently usable and reproducibly
packageable on the validated environments. Confirmed root/sqrt issues are resolved
with regression evidence, and remaining accuracy/platform constraints are visible.
Stable release readiness is not claimed solely because the finite tests pass.
