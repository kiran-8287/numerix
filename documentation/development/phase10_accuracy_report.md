# Phase 10 — accuracy and convergence hardening

Date: 2026-11-12. Base: e36f3f2bed7f7ef3af14534e4eadd93951b11cb9.
Phase 9 was confirmed clean and already pushed to origin/master; its old failed
push report is historical. Its measured results remain unchanged in phase9/.

## Findings and mathematical changes

1. Brent, Newton and secant previously accepted a small nonzero residual as a
   position tolerance; Newton/secant also rejected small-scaled derivatives or
   differences against an absolute epsilon floor. Root shared options now have
   position units. Sign-changing methods preserve their bisection/chord/IQI/Newton
   rules and check position enclosures. Newton/secant use observed contracting
   steps with an estimated remaining distance, requiring decreasing residuals;
   this is local heuristic evidence, not a certified bound. Zero derivative or
   degenerate secant differences remain numerical failures; stagnation is a
   convergence failure and budget exhaustion LIMIT_REACHED. Scale normalization
   avoids an absolute function-unit denominator threshold. Exact computed zeros
   are still accepted; underflow/cancellation in callbacks cannot be undone.
2. Brent previously returned a bracket endpoint with a half-width estimate,
   although a half-width bounds midpoint error, not arbitrary endpoint error.
   Endpoints now require the whole width; bisection bounds its actual returned
   midpoint. Managed Bisection no longer uses half of an already updated bracket
   to certify its previous midpoint. Large opposite-sign bounds use a midpoint
   calculation that avoids overflowing their difference.
3. Recursive adaptive Simpson divided tolerance by two at every depth. Near
   sqrt(x)'s endpoint, error scales as h^(3/2) while that local budget scales as
   h, forcing unnecessary depth even when total error could meet the request.
   The same Simpson/Richardson rule now refines the largest-error panel using
   a heap and a global sum, bounded by the unchanged depth parameter (1..64),
   evaluation and split budgets. It uses the full successive-rule difference
   rather than the smooth-only /15 estimate, plus a roundoff floor. The /15
   correction remains in the integral. This estimate is conservative on the
   tested functions, not a universal bound; unsampled oscillations can fool any
   finite sampling strategy. No recursion/stack-depth exhaustion remains.
4. Fixed-rule accumulation retains compensation, using extended precision where
   available. Non-finite results/estimates fail explicitly. Depth/split/evaluation
   exhaustion and inability to meet roundoff targets are distinguished.
5. Discovery refinement no longer converts its independent residual threshold
   into a position tolerance. Sample-only roots expose NAN position estimates;
   merging never substitutes function residual units for position error units.
   Selection/fallback accepts zero or adequate position evidence, not a small
   residual alone. Discovery remains heuristic with explicit function-unit
   residual and merge tolerances; badly scaled thresholds can still produce
   weak candidates and cannot prove location accuracy/completeness.
6. Root diagnostics no longer derive a residual threshold from x tolerances.
   Unavailable thresholds/metrics are flagged and initialized to NAN; position
   estimates remain separate. Historical recommendations require an available
   threshold before interpreting its pass/fail flag. No public fields or
   signatures changed. The status/stopping behavior intentionally changed.

All modified numerical functions use conventional multi-line C. Necessary
formatting inside the two materially changed numerical modules was performed;
no repository-wide formatting-only cleanup was undertaken.

## Regression and integration coverage

`tests/test_accuracy.c` checks all six root methods against 11 scale/power/
exponential cases (66 solver/problem pairs), with analytical position and
normalized-residual checks. It includes scales 1e-200 through 1e200, linear,
cubic and ninth-order flat roots, exp(x)-3, an endpoint root, iteration limits,
stagnation, a discontinuous sign jump, and invalid/tiny/large tolerance options.

Integration cases include constant, quartic, trigonometric, exponential,
square-root endpoint, non-finite integrand, improper endpoint, oscillation,
reversed bounds, tight roundoff requests, depth and evaluation/split limits.
Automatic selection, managed execution, diagnostics and scaled discovery with
an explicitly scaled residual threshold are exercised together. Existing tests
cover multiple/tangential/repeated/close/endpoint/piecewise roots, poles,
unresolved intervals, cancellation and fallback. Allocation fault injection now
also covers every observed adaptive Simpson allocation/growth point.

## Measured accuracy

Same analytical cases and requested tolerances; see
[accuracy output](../../benchmarks/results/phase10/accuracy.txt),
[CSV](../../benchmarks/results/phase10/measurements.csv),
[allocation probes](../../benchmarks/results/phase10/allocations.csv), and
[complete summary](../../benchmarks/results/phase10/summary.md).

| Case | Phase 9 NUMERIX absolute error | Phase 10 absolute error |
| --- | ---: | ---: |
| Flat cubic, Brent | 4.13e-4 | 5.27e-12 |
| Flat cubic, Newton | 3.38e-4 | 1.03e-10 (within its 1.3e-10 position tolerance) |
| 1e-14-scaled quadratic, Brent | 0.414 | 7.68e-14 |
| 1e-14-scaled quadratic, Newton | 0.414 | 0 at the rounded analytical reference |
| Integral sqrt(x), [0,1], depth32 | convergence failure | 4.18e-13; estimated error 1.00e-10; 829 evaluations |

All root benchmark accuracy checks now pass for successful valid solves in both
libraries. The GSL benchmark driver was updated to position/contraction evidence;
it no longer emulates the removed residual shortcut. New CSVs must not be used
as a pure speed-optimization comparison with the old driver. No historical
measurements were overwritten and no tolerances were loosened.

The 222-case suite retains 10 nonzero statuses (invalid brackets and numerical
Hilbert rejections) and 13 successful-call misses (Hilbert forward error and
fixed quadrature discretization). These are explicit remaining limitations, not
hidden successes. Allocation output ends at zero tracked live bytes in all cases.

## Validation and completion gate

- Passed: GCC Debug full 32 CTests; GCC Release with -Werror full 33 CTests,
  including optional allocation faults; clean Make build/full tests -O2 -Werror.
- Passed: 222 GSL/NUMERIX timing cases, allocation probes, CSV verification and
  all NUMERIX capability cases in that suite. GSL 2.8 remains optional.
- Passed: selected allocation-failure cleanup including expression, six discovery
  traces and adaptive integration.
- Phase 11 owns the comprehensive cross-compiler/sanitizer/native platform rerun;
  no new Windows/MSVC result is claimed here.

Gate satisfied: confirmed convergence issues fixed, endpoint behavior investigated,
regressions and integration tests run, existing suites intact, documentation
updated. Phase 10 is committed separately before Phase 11 begins. The exact
commit hash is recorded in the final completion response.

## Remaining mathematical limits

Open-method contraction estimates are heuristic, callbacks can lose information,
continuity/derivative assumptions cannot be proved, and discovery is incomplete.
Closed finite-endpoint quadrature rejects an infinite sample such as 1/sqrt(x)
at zero; callers must reformulate improper integrals. Error estimates are not
rigorous enclosures, arbitrarily tight accuracy is not available in binary64,
and fixed-grid or ill-conditioned cases retain their documented limitations.
