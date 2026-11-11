# Phase 2 report

## Architecture inspected

Before implementation I inspected the public header, core and execution-context implementation, Phase 1 algorithm sources, memory-management notes, existing tests, CMake, and Make. The library exposes a scalar callback returning `double`, shared status/error types, and private implementation headers. I preserved the callback signature and numerical algorithm APIs. The expression subsystem is added under `src/expression/` and its internal AST/token definitions are private.

## Files created

- `src/internal/numerix_expression_internal.h`
- `src/expression/lexer.c`
- `src/expression/parser.c`
- `src/expression/evaluator.c`
- `src/expression/expression.c`
- `tests/test_expression.c`
- `examples/expression_demo.c`
- `documentation/api/expression_engine.md`
- `documentation/development/phase2_report.md`

## Files modified

- `include/numerix.h`: opaque expression API and callback adapter declarations.
- `CMakeLists.txt`: expression sources, demo, test executable, and CTest registration.
- `Makefile`: expression sources, demo, tests, and cleanup integration.
- `.gitignore`: added native expression binary names.
- `README.md`, architecture overview/data-structure/memory documentation.

Phase 1 algorithm implementation files were not changed during Phase 2.

## Lexer, parser, and AST

The lexer recognizes finite decimal/scientific literals, identifiers, operators, parentheses, commas, and whitespace. It reports zero-based character positions for lexical and syntax errors. The recursive-descent parser constructs a private AST with number, mathematical-constant, variable, unary, binary, and function nodes. Tree depth and source/node counts are bounded to limit stack and allocation use.

Precedence is parentheses/functions, right-associative exponentiation, unary signs, multiplication/division, then addition/subtraction. Thus `-2^2` is `-4`, `(-2)^2` is `4`, `2^-2` is valid, and `2^3^2` is `512`.

## Supported operations and functions

Arithmetic: `+`, `-`, `*`, `/`, `^`, parentheses, unary signs, and variable `x`. Constants: `pi`, `e`. Unary math functions: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `abs`, `exp`, `log`, `log10`, `floor`, and `ceil`. Binary functions: `pow`, `min`, and `max`.

The evaluator detects division by zero, invalid square-root/log/inverse-trigonometric/power domains, and non-finite arithmetic. Syntax/domain issues use existing status/error types and include a source position when associated with a token.

## Public API and callback integration

`numerix_expression_compile`, `numerix_expression_evaluate`, and `numerix_expression_destroy` manage opaque compiled expressions. Expressions are immutable after compilation. `numerix_expression_callback_context_init` initializes caller-owned adapter state; `numerix_expression_callback` can then be passed directly to the existing root, integration, or differentiation APIs. The adapter returns `NAN` on failure to trigger existing callback failure behavior and retains the expression-specific status/error in its context. The expression must outlive the call, and each concurrent call needs a separate callback context.

## Validation results

Environment: Windows with MinGW GCC 6.3.0. The complete Make build used `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow` and completed without warnings.

- `mingw32-make clean` — passed.
- `mingw32-make all` — passed; static library, CLI, and expression demo built.
- `mingw32-make test` — passed Phase 0 unit/integration tests, all seven Phase 1 test categories, and the expression suite (`All 3138 expression checks passed.`).
- `.\numerix_expression_demo.exe` — passed. Root example returned `2.0000000168`; integral of `sin(x)` on `[0, pi]` returned `2.0000000000`; derivative of `x^3` at 2 returned `12.00000001`.
- `.\numerix_cli.exe` — passed and confirmed initialization.
- `git diff --check` — passed; no whitespace errors.
- Address/undefined sanitizer compilation was attempted. MinGW GCC 6.3.0 aborted with an internal compiler error in `parser.c` while compiling with `-fsanitize=address,undefined`; no sanitizer executable was produced or run.
- CMake/CTest executables are unavailable in the environment, so their configured build and test paths were not executed. Linux and Clang were also unavailable.

## Limitations

Only `x` is a variable. Implicit multiplication, user-defined functions, comparisons, modulo, piecewise expressions, and multivariable expressions are not supported. Numeric literal conversion uses standard `strtod`; callers should use the C numeric locale for decimal-point input. The existing scalar callback cannot directly return a status, so callback failures appear to algorithms as non-finite callback results; inspect the adapter context for the specific expression error. The callback context is mutable and must not be shared by concurrent calls. Sanitizer, CMake/CTest, Linux, and Clang verification remain outstanding.

No Phase 3 functionality was started.
