# All-module combined figures

These three figures bring together only the methods with recorded evidence.

- [Combined runtime — PNG](runtime_all_modules.png) · [SVG](runtime_all_modules.svg): 56 timed routes. Root data are historical Phase 14B; other timings are Phase 14C pilots. Optimization and ODE have no timing runs. Different operations and workloads prevent a cross-module speed ranking.
- [Combined memory status — PNG](memory_all_modules.png) · [SVG](memory_all_modules.svg): all seven modules are explicitly marked **not measured** for method-specific memory. No byte values are fabricated from smoke controls or RSS.
- [Combined accuracy overview — PNG](accuracy_all_modules.png) · [SVG](accuracy_all_modules.svg): evaluated target pass rates are shown by route; the ODE panel uses normalized analytic trajectory error because no pass threshold was set. Setup-only interpolation construction has no numerical-output accuracy value.

Supporting route data:

- [`combined_runtime_routes.csv`](combined_runtime_routes.csv)
- [`combined_accuracy_routes.csv`](combined_accuracy_routes.csv)
- [`combined_memory_status.csv`](combined_memory_status.csv)
- [`verification.json`](verification.json)

Accuracy targets differ by module, so the combined plot is an evidence overview, not a ranking across modules. See the [executed-method inventory](../README.md) for the exact methods, run IDs, sample counts, and copied raw data.
