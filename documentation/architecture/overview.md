# Architecture overview

NUMERIX is a C11 static library with a single public entry header, `include/numerix.h`. Callers depend on that header; implementation code may depend on private declarations under `src/internal/`. Internal structures are not part of the supported ABI.

## Module responsibilities

- **Core (`src/core`)**: statuses, errors, options, scalar callback handling, allocation helpers, initialization, and the execution-context foundation.
- **Algorithms (`src/algorithms`)**: Phase 1 implementations grouped into root finding, linear algebra, differentiation, interpolation, integration, optimization, and ODE modules.
- **Expression (`src/expression`)**: planned tokenization, parsing, validation, and evaluation.
- **Piecewise (`src/piecewise`)**: planned representation of piecewise functions and discontinuities.
- **Intelligence (`src/intelligence`)**: planned problem assessment and method recommendation.
- **Execution (`src/execution`)**: planned computation control; currently only core context state and counters exist.
- **Diagnostics (`src/diagnostics`)**: planned numerical issue analysis.
- **CLI (`cli`)**: a small linked startup demonstration, not an algorithm interface.

## Data flow and boundaries

Applications create a public problem and options, then pass them into public operations. The callback retains its caller-owned `user_data`; the library does not take ownership. The context copies validated options and owns its own internal state. Future algorithms should consume public problem descriptions and an execution context while keeping algorithm-specific state private or in specialized result types.

Core must not depend on algorithm modules. Algorithm families may depend on core types. Expression and piecewise implementations can later adapt their representations to the same problem-specific APIs without forcing matrices, datasets, and differential equations into the scalar callback structure.

## Current scope

Phase 1 implements the algorithms listed above. Expression, piecewise, intelligence, automatic fallback, and advanced diagnostics remain future work. Reserved directories do not indicate implemented functionality.
