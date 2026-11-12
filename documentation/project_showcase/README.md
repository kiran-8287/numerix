# NUMERIX Project Showcase

This folder contains a self-contained, static technical showcase website for
NUMERIX, the C11 numerical-computation library. It is a separate website from
`documentation/presentation/` and reuses that presentation's visual style,
typography, spacing, navigation approach, card design, colors, and responsive
behavior. The original presentation folder is unchanged.

## Purpose

The showcase explains the complete NUMERIX project:

- What the original project was supposed to achieve and how NUMERIX evolved
- What NUMERIX is and the seven numerical modules it covers
- What makes NUMERIX different from a conventional numerical-methods library
- The complete architecture
- Every major feature implemented during the redesign (20 features)
- Every numerical module and its algorithms
- How users interact with the library (callbacks and expression strings)
- How piecewise functions are represented and evaluated
- How intelligent algorithm selection works
- How execution monitoring and diagnostics work
- How to write and compile actual C programs using NUMERIX
- How the library handles errors, failures, and memory ownership
- How the project is tested, built, installed, and reused

Every feature is verified against the actual public API in `include/numerix.h`
and the implementation under `src/`. Every C example was compiled and executed
against `libnumerix.a`, and its output is shown on the site.

## Open the website

This is a static website. No web server, backend, account, or internet
connection is required. Open `index.html` in a modern browser:

```bash
# Linux
xdg-open documentation/project_showcase/index.html

# macOS
open documentation/project_showcase/index.html
```

```powershell
# Windows PowerShell
Start-Process .\documentation\project_showcase\index.html
```

Keep `assets/showcase.css` and `assets/showcase.js` beside `index.html`. The
site loads no external resources.

## What was NOT copied

The benchmark system was intentionally excluded:

- No benchmark datasets, CSV, or JSON files
- No benchmark dashboards or graphs
- No benchmark-specific JavaScript
- No benchmark result images or administration controls
- No large comparison selector or dropdown interface

## Files

- `index.html` — complete showcase content (12 sections, 20 features, 7 modules,
  12 tutorials, build/test sections, conclusion)
- `assets/showcase.css` — styles (reuses the presentation visual language)
- `assets/showcase.js` — interactivity (copy buttons, tabs, mobile menu,
  progress indicator, anchor navigation)
- `README.md` — this file

## Verification

A content verification report is maintained separately and lists every feature,
its source/API reference, implementation status, limitations, and whether each C
example was compiled and executed successfully.

## Related

- Original presentation: `documentation/presentation/`
- NUMERIX source: `include/numerix.h`, `src/`
- Build/test reports: `documentation/development/`
- Public API guides: `documentation/api/`
