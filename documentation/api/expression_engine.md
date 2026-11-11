# Mathematical expression engine

NUMERIX compiles a scalar mathematical expression once into a private abstract syntax tree (AST), then evaluates that tree for any finite value of `x`. The AST is opaque and immutable after compilation. Evaluation does not modify it, so a compiled expression may be evaluated repeatedly or shared for concurrent read-only evaluations.

## Supported syntax

The lexer recognizes decimal and scientific-notation literals (for example `.75`, `5.`, `1.5e-3`, and `2.5E+3`), the variable `x`, constants `pi` and `e`, whitespace, `+`, `-`, `*`, `/`, `^`, parentheses, and commas in function calls.

The parser uses this precedence, from tightest to loosest:

1. Parenthesized expressions and function calls.
2. Exponentiation, right associative.
3. Unary positive and negative signs.
4. Multiplication and division, left associative.
5. Addition and subtraction, left associative.

The unary/exponent rule means `-2^2` is `-(2^2)` and evaluates to `-4`, while `(-2)^2` is `4`. A signed exponent is accepted, so `2^-2` is valid. Exponentiation chains associate right, so `2^3^2` is `2^(3^2)`.

Unary functions: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `abs`, `exp`, `log`, `log10`, `floor`, and `ceil`. Binary functions: `pow`, `min`, and `max`. Names are lowercase and argument counts are checked at compile time.

Implicit multiplication, other variables, modulo, comparisons, and user-defined functions are not supported. Source text is limited to 4096 characters, the AST to 4096 nodes, and AST depth to 256 nodes to bound resource use.

## Compilation, evaluation, and ownership

`numerix_expression_compile` validates its pointers, lexes, parses, and returns an opaque expression through the output pointer. The output is set to `NULL` on failure. The caller owns a successful expression and releases it with `numerix_expression_destroy`; destroying `NULL` is safe.

`numerix_expression_evaluate` accepts a finite x and output pointer. It writes the result only on success. Syntax and domain failures use `NUMERIX_STATUS_INVALID_INPUT`; pointer and non-finite x failures use `NUMERIX_STATUS_INVALID_ARGUMENT`; arithmetic overflow and non-finite math results use `NUMERIX_STATUS_NUMERICAL_FAILURE`. Error messages include a zero-based source character position when tied to a token. Division by zero, negative square roots, nonpositive logarithm inputs, invalid inverse-trigonometric arguments, invalid powers, overflow, and underflow are detected.

`errno` may be used by the standard math functions for range detection. The evaluator clears it before each function call. Compile-time numeric literal conversion uses `strtod`, as specified by C; applications should keep the C numeric locale when compiling expressions that use a decimal point.

```c
numerix_expression *expression = NULL;
numerix_error error;
double value;

numerix_status status = numerix_expression_compile(
    "x^2 + 3*x - 5", &expression, &error);
if (status == NUMERIX_STATUS_SUCCESS)
{
    status = numerix_expression_evaluate(expression, 2.0, &value, &error);
    numerix_expression_destroy(expression);
}
```

## Callback integration

The existing algorithm callback returns only a `double`, so it cannot return a status directly. Initialize a caller-owned `numerix_expression_callback_context` with a compiled expression and pass `numerix_expression_callback` plus that context to a Phase 1 scalar algorithm. On an expression error, the adapter returns `NAN` so the numerical method stops through its existing non-finite callback handling. Inspect `numerix_expression_callback_status` and `numerix_expression_callback_error` to retrieve the underlying expression status and message.

The callback context borrows its expression, so the expression must outlive the entire algorithm call. Each context is mutable while it records the latest evaluation status; do not share one context across simultaneous algorithm executions. The expression itself is immutable and may be shared.

```c
numerix_expression *expression = NULL;
numerix_expression_callback_context callback_context;
numerix_result root;
numerix_error error;

numerix_status status = numerix_expression_compile("x^2 - 4", &expression, &error);
if (status == NUMERIX_STATUS_SUCCESS)
{
    status = numerix_expression_callback_context_init(
        &callback_context, expression, &error);
}
if (status == NUMERIX_STATUS_SUCCESS)
{
    status = numerix_root_brent(
        numerix_expression_callback, &callback_context,
        0.0, 3.0, NULL, &root, &error);
}
numerix_expression_destroy(expression);
```

When an algorithm reports a generic callback numerical failure, the adapter context preserves the more specific expression error. The adapter does not use global state and remains compatible with existing callback-based APIs.
