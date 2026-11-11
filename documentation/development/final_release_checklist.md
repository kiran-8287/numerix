# Final release checklist — 0.2.0-rc1

Date: 2026-11-12. Assessment: **release candidate with documented limitations**.
Phase 10 and Phase 11 were committed before the next milestone began. Historical
Phase 9 metrics/reports remain intact; this checklist describes the current candidate.

| Area | Actual classification | Evidence / limit |
| --- | --- | --- |
| Root accuracy/convergence | Passed |66 method/problem regression pairs; flat/scaled roots fixed; all successful valid benchmark root rows meet their position check |
| Adaptive Simpson | Passed |sqrt endpoint error 4.18e-13 at 1e-10 request, depth 32; global budget, no depth increase; roundoff/nonfinite/limit cases tested |
| Remaining mathematics | Partially passed |Open-method/quadrature estimates and discovery are heuristic; improper infinite endpoint samples unsupported; ill-conditioning/fixed-grid misses retained |
| Make | Passed |Clean Debug/Release full builds/tests, PREFIX/DESTDIR basic installation, no mandatory GSL |
| CMake/CTest | Passed |Clean Debug/Release GCC/Clang;33 standard tests,34 with optional allocation faults; declared minimum3.16.8 actually built/tested |
| Linux compilers | Passed |GCC 8.5/11.5/15.2 and Clang 21.1; matching toolset binutils where required |
| Windows MinGW | Unavailable execution |Native CI requested, runs queued with no job results; old6.3 results historical |
| Windows MSVC | Unavailable execution |C11 atomics feature check/flag and CI prepared; no native result claimed |
| macOS | Unavailable execution |Apple Clang CI requested, no job results available |
| Unit/integration/regressions | Passed |All existing suites plus accuracy, cross-phase, layout, CLI and example tests |
| Sanitizers | Passed |ASan+UBSan+detect_leaks on34 tests; no reported errors |
| Memory analysis | Passed |Valgrind on full12 test executables and allocation/fault suites; zero errors/leaks in executed traces |
| Allocation failures | Passed |Selected expression, discovery and adaptive Simpson allocation/growth indices; not exhaustive whole-library coverage |
| Static analysis | Passed |GCC 15 analyzer, Clang analyzer and exhaustive Cppcheck; five earlier test-harness findings fixed without suppressions |
| API audit | Passed |All126 public function declarations have archive definitions; ownership/options/errors documented; matching C/C++ headers/clients |
| ABI policy | Passed policy; stability not established |0.x minor compatibility/rebuild policy documented; no stable/shared ABI promise |
| CMake installation | Passed |Relocatable NUMERIX::numerix export, version file, header/archive/license/docs; GSL absent from library symbols/references |
| Independent consumers | Passed |Separate copied CMake project, C/C++ Debug/Release after prefix relocation; roots/LU/expressions/errors/cleanup |
| pkg-config | Passed |Relocated metadata compiles/runs a real static C consumer; math linkage included with --static |
| Version policy | Passed |0.2.0-rc1, numeric CMake 0.2.0; prior minor request rejected; unsupported shared build explicitly rejected |
| README/API/user guides | Passed |Updated implemented capabilities, build/install/quick start, compatibility and limitations; packaged local links checked |
| Examples/CLI | Passed |All five examples, startup/help, expression/root/discovery/piecewise demos and invalid/domain input checks |
| Source distribution | Passed |Curated source-only manifest; deterministic archive reproduced twice and from a Git-free extraction; no local paths/tokens/build objects/history logs |
| Extraction/rebuild | Passed |Extracted source CMake and Make Debug/Release, full suites; binary archive extracted and C/C++ consumers run |
| Binary distribution | Passed Linux only |Release static-library TGZ/checksum for validated Linux x86_64; no unvalidated-platform binaries |
| License | Passed |MIT license preserved and installed; external GSL sources/binaries not shipped |
| Git/source review | Passed |Separate milestone commits; full diff/source lists/ignore rules/artifacts reviewed; final exact hashes/tree/push in completion response |
| Public release/tag/upload | Not attempted |Artifacts prepared locally under dist/; no stable tag or GitHub release assets published |

## Release artifacts and reproducibility

`dist/numerix-0.2.0-rc1-source.tar.gz` and
`dist/numerix-0.2.0-rc1-Linux-x86_64.tar.gz`, each with a SHA256 sidecar.
[Artifact hashes/sizes](../../benchmarks/results/phase12/artifacts.json) and
[validation evidence](../../benchmarks/results/phase12/) are retained separately
from distributable source. `scripts/package_source.py` recreates the curated
source package; CPack produces the build-platform binary package.

## Before a stable release

Obtain and review actual Windows/MSVC/macOS execution results, broaden numerical
problem/hardware coverage, continue adversarial/scaling tests and review the
new tolerance contract with consumers. Keep discovery/conditioning/error estimates
qualified and define/test a binary-compatibility contract before promising one.
A green finite suite does not establish universal accuracy or portability.
