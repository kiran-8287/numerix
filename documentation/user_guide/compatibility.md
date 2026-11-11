# Compatibility and versioning policy

The candidate is **0.2.0-rc1** (numeric CMake package version0.2.0). It follows
0.1.0 with corrected root tolerance/status behavior and adaptive integration.
Public function signatures and struct layouts were preserved in Phases10–12;
behavioral compatibility is intentionally changed where earlier answers were
incorrect. Read [tolerance semantics](../api/tolerances.md).

- **API** is the declared functions, types, options, ownership and behavior.
- **Source compatibility** means a client can be rebuilt against the new header.
- **ABI/binary compatibility** additionally requires matching layouts, enum sizes,
  calling conventions, architecture, runtime allocation conventions and symbols.
  A successful source rebuild does not establish binary compatibility.
- **0.x policy:** minor versions may change public contracts/layouts. Patch
  versions should preserve documented source contracts and public layouts; incompatible
  layout/calling-convention changes require a minor bump. Pre-release
  suffixes identify candidates, not stable guarantees. CMake accepts only the same
  minor version when a version is requested. Future incompatibilities require a
  minor bump while major is zero, with rebuild/migration notes.
- **Future 1.x policy:** breaking public API/ABI changes require a major release;
  ABI stability must first be explicitly defined and validated, not inferred.

There is **no stable ABI promise** for0.2.0. Phase 8 expanded diagnostics/search
structures. Rebuild precompiled older clients and use the matching header/archive.
The current supported distribution is static, with NUMERIX allocations destroyed
by NUMERIX routines. Shared libraries are explicitly unsupported rather than
silently ignoring BUILD_SHARED_LIBS. Do not mix incompatible compiler/CRT builds.
No public layout is serialized or suitable as an on-disk/network format.

C11 source builds are validated on the recorded Linux GCC/Clang configurations.
Windows/MSVC/macOS remain unvalidated while hosted jobs have no execution result.
MSVC requires supported C11 atomics and its checked experimental compiler flag.
C++ clients include the same public header through extern-C declarations; the
independent C and C++ installed consumers test actual linkage and cleanup.

CMake version matching enforces a source-version policy, not cross-compiler ABI
certification. Its numeric version does not distinguish pre-release suffixes;
inspect NUMERIX_VERSION_STRING and the candidate assessment before deployment.
