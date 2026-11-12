# Presentation verification report

**Date:** 2026-11-12  
**Scope:** `documentation/presentation/` only

This report records checks against the presentation assets and their benchmark
source data. It does not certify the NUMERIX library beyond the separately
recorded build/test reports. No historical benchmark input or report was
modified by the presentation data builder.

## Benchmark-data checks

| Check | Result |
| --- | --- |
| Regenerate local presentation payload | **Passed** — `python3 documentation/presentation/scripts/build_benchmark_data.py` verified the source checksums and row counts, then wrote the presentation JSON/JS assets. |
| Parse generated JSON and verify coverage | **Passed** — JSON parsed; 17 library/method routes, 1,700 problem records, seven module coverage entries and five direct paired comparisons were present. |
| Parse generated JavaScript | **Passed** — `node --check documentation/presentation/data/phase14b_root_comparisons.js` completed successfully. |
| Direct ratios and paired counts | **Passed** — reproduced from the preserved paired comparison CSV: Bisection 94 / 1.0818; Regula Falsi 95 / 1.0241; Secant 85 / 1.0754; Newton 90 / 1.0694; Brent 95 / 1.0372. Ratio is GSL median time divided by NUMERIX median time. |
| Incomplete measurement disclosure | **Passed** — payload records memory, tolerance sweep and time-to-accuracy as unavailable; the six other module studies remain pending. |
| Historical inputs | **Passed** — the builder writes only `documentation/presentation/data/`; the Phase 14B source CSVs and reports were read and checksum-verified. |

The five ratios and intervals are descriptive paired-problem summaries, not
statistical significance claims. Regula Falsi's descriptive interval includes
one. Runtime/error points use one fixed tolerance and must not be presented as
a tolerance-sweep or time-to-accuracy frontier. Method-specific memory is not
available; process RSS is not substituted.

## HTML, layout, and browser checks

| Check | Result |
| --- | --- |
| HTML validation | **Pending** |
| CSS and page JavaScript syntax | **Pending** |
| Internal anchors and local assets | **Pending** |
| Desktop and narrow-screen layout | **Pending** |
| Section navigation, keyboard controls and progress indicator | **Pending** |
| Full-screen enter/exit | **Pending** |
| Graph labels, units, legends, counts and chart-to-source checks | **Pending** |
| Local `file://` open and offline interaction | **Pending** |
| Presentation timing/readability | **Pending** |

These checks are completed only after the final `index.html`, CSS and
JavaScript are present and the page is exercised in a browser. A successful
data-generator run alone does not validate the presentation UI or graphs.

## Remaining data limits

- Phase 14B method-level measurements cover root routes only. Linear algebra,
  differentiation, interpolation, integration, optimization and ODE remain
  pending.
- Only five root methods have direct paired GSL/NUMERIX timing analyses.
- Safeguarded root solving, CMINPACK's hybrid system solver and multiple-root
  discovery use best-available comparisons where shown; they are not identical
  algorithm comparisons.
- Method-specific memory and tolerance-sweep/time-to-accuracy results are not
  available.

Source details are in [`data/README.md`](data/README.md), the
[Phase 14B evaluation](../../documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md)
and the saved [analysis verification manifest](../../benchmarks/results/phase14b/analysis/root/analysis_verification.json).
