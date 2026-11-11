# Change log

## 0.2.0-rc1

- Correct root stopping to separate position accuracy from function residuals;
  preserve bracket enclosures, scale normalized secant differences, detect
  stagnation and report resource limits distinctly.
- Use global adaptive Simpson error allocation with bounded iterative workspace;
  resolve the tested sqrt endpoint failure without increasing its depth limit.
- Correct root diagnostics/discovery metric units and availability.
- Add accuracy regressions, compiler/layout probes, robust test sentinels,
  adaptive allocation-failure tests and hosted native validation definitions.
- Export relocatable CMake and pkg-config packages; support installed C/C++
  clients, Make staging and clean source distributions with checksums.
- Document the 0.x compatibility policy and remaining candidate limitations.

Function signatures/layouts remain unchanged from0.1.0. Root stopping/status and
integration resource/error-estimate behavior intentionally changed. Rebuild older
clients; no stable ABI is promised. Windows/macOS hosted execution remains pending.

## 0.1.0 development history

Phases0–9 implemented the numerical library, expressions, piecewise functions,
recommendation/fallback, managed execution, diagnostics/sensitivity, heuristic
root discovery, allocation hardening and reproducible GSL evaluation. Historical
reports and raw observations are retained in the source repository.
