# NUMERIX technical presentation

This folder contains the self-contained presentation for a live technical
demonstration of NUMERIX. It is a static website: it needs no web server,
backend, account or internet connection.

## Open the presentation

Open [`index.html`](index.html) in a modern browser. From the repository root,
you can also launch it from a terminal:

```bash
# Linux
xdg-open documentation/presentation/index.html

# macOS
open documentation/presentation/index.html
```

```powershell
# Windows PowerShell
Start-Process .\documentation\presentation\index.html
```

Keep the local `assets/` and `data/` files beside the page. The benchmark data
is loaded from a local JavaScript file so it remains available when the page is
opened with a `file://` URL; the presentation does not fetch JSON over HTTP.

## Navigate and present

Use the compact section navigation or the Previous/Next controls to move
through the presentation. Keyboard shortcuts and the full-screen control are
shown in the page. Press Escape to leave browser full-screen mode. The progress
indicator tracks the current section. Browser full-screen permission is handled
by the browser and may require a click on the full-screen control.

For a live talk, use a desktop browser at presentation size, enter full screen,
and move through the opening, motivation, architecture, seven module sections,
developer workflow, demonstrations, validation, method-level benchmark
comparisons and conclusion. The content and charts are designed to remain
readable at smaller screen sizes as well.

## Benchmark data and regeneration

The page reads [`data/phase14b_root_comparisons.js`](data/phase14b_root_comparisons.js)
and its reviewable JSON companion
[`data/phase14b_root_comparisons.json`](data/phase14b_root_comparisons.json).
Regenerate both from the preserved Phase 14B root-study inputs with:

```bash
python3 documentation/presentation/scripts/build_benchmark_data.py
```

The generator verifies source checksums and row counts before writing only the
presentation data folder. Its source inventory and exact output interpretation
are documented in [`data/README.md`](data/README.md). It reads the v3
`method_summary.csv`, `paired_comparisons.csv`, analysis verification manifest,
raw accuracy and timing CSVs, workload map, run metadata and seven-module method
inventory.

The output contains 17 measured library/method routes and five direct GSL versus
NUMERIX paired comparisons. Each paired ratio uses eligible problem-level
medians and the saved descriptive bootstrap interval. Only root routes have
Phase 14B method-level measurements. The other six modules are listed as
pending; this is not a seven-module performance ranking.

No method-specific memory measurements or tolerance-sweep/time-to-accuracy
measurements exist for this study. Process RSS from another study is not
substituted for method memory. The generated problem-level runtime/error points
are from one fixed tolerance; they are not an accuracy-versus-time tuning curve.
The GSL safeguarded route, CMINPACK hybrid solve and GSL multiple-root scan are
best-available comparisons with different algorithms and are labeled as such.
See the [Phase 14B evaluation](../../documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md)
and the [data notes](data/README.md) before presenting results.

## Updating after a new benchmark study

1. Preserve the existing phase data and create a separate, immutable output
   directory for the new run.
2. Complete the workload, accuracy, timing and analysis verification for that
   run. Confirm its manifest and checksums before exposing it in the page.
3. Update `scripts/build_benchmark_data.py` to read the new verified inputs and
   run identifier. Add schema checks for every newly measured route; do not
   replace missing or unsupported values with zero.
4. Regenerate the JSON and JavaScript presentation payload, then cross-check its
   counts, medians, accuracy coverage, paired ratios and intervals against the
   analysis CSVs and report.
5. Update the page's source links and limitations. Label direct versus
   best-available comparisons, and keep unmeasured methods visibly pending.
6. Run the HTML, script, chart, internal-link and offline-browser checks recorded
   in [`verification_report.md`](verification_report.md).

Do not edit or overwrite the original benchmark CSVs or Phase 13/14 reports to
make the presentation fit a chart. Correct any analysis issue in a separately
documented run while retaining its original inputs.

## Files

- `index.html` — presentation content and section structure.
- `assets/` — local styles, interaction code and diagrams.
- `data/` — generated benchmark payload plus its reproducible builder notes.
- `scripts/build_benchmark_data.py` — verifies and generates the local chart data.
- [`verification_report.md`](verification_report.md) — checks performed and
  remaining platform/browser validation.

The presentation is a communication layer over NUMERIX documentation and
verified study artifacts. It does not change the public API, numerical code or
historical benchmark record.
