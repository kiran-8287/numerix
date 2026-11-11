#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t checks_run;
static size_t checks_failed;

#define CHECK(condition) \
    do \
    { \
        ++checks_run; \
        if (!(condition)) \
        { \
            ++checks_failed; \
            fprintf(stderr, "%s:%d: check failed: %s\n", \
                    __FILE__, __LINE__, #condition); \
        } \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance) \
    CHECK(fabs((actual) - (expected)) <= \
          (tolerance) * fmax(1.0, fabs(expected)))

static double evaluate_text(const char *source, double x)
{
    numerix_expression *expression = NULL;
    numerix_error error;
    double value = NAN;
    numerix_status status;

    status = numerix_expression_compile(source, &expression, &error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "compile failed for '%s': %s\n", source, error.message);
        ++checks_run;
        ++checks_failed;
        return NAN;
    }

    status = numerix_expression_evaluate(expression, x, &value, &error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "evaluation failed for '%s': %s\n", source, error.message);
        ++checks_run;
        ++checks_failed;
    }

    numerix_expression_destroy(expression);
    return value;
}

static void test_numeric_literals_and_arithmetic(void)
{
    CHECK_NEAR(evaluate_text("2 + 3", 0.0), 5.0, 1e-14);
    CHECK_NEAR(evaluate_text("10 - 4", 0.0), 6.0, 1e-14);
    CHECK_NEAR(evaluate_text("3 * 5", 0.0), 15.0, 1e-14);
    CHECK_NEAR(evaluate_text("20 / 4", 0.0), 5.0, 1e-14);
    CHECK_NEAR(evaluate_text("2^3", 0.0), 8.0, 1e-14);
    CHECK_NEAR(evaluate_text("1.5e-3 + 2.5E+3", 0.0), 2500.0015, 1e-12);
    CHECK_NEAR(evaluate_text(".75 + 5.", 0.0), 5.75, 1e-14);
}

static void test_precedence_and_variables(void)
{
    CHECK_NEAR(evaluate_text("2 + 3 * 4", 0.0), 14.0, 1e-14);
    CHECK_NEAR(evaluate_text("(2 + 3) * 4", 0.0), 20.0, 1e-14);
    CHECK_NEAR(evaluate_text("2^3^2", 0.0), 512.0, 1e-14);
    CHECK_NEAR(evaluate_text("-x", 3.0), -3.0, 1e-14);
    CHECK_NEAR(evaluate_text("-2^2", 0.0), -4.0, 1e-14);
    CHECK_NEAR(evaluate_text("(-2)^2", 0.0), 4.0, 1e-14);
    CHECK_NEAR(evaluate_text("x^2 + 2*x + 1", 3.0), 16.0, 1e-14);
    CHECK_NEAR(evaluate_text("2^-2", 0.0), 0.25, 1e-14);
    CHECK_NEAR(evaluate_text("--x", 3.0), 3.0, 1e-14);
}

static void test_constants_and_functions(void)
{
    const double pi = 3.14159265358979323846;

    CHECK_NEAR(evaluate_text("pi", 0.0), pi, 1e-15);
    CHECK_NEAR(evaluate_text("e", 0.0), exp(1.0), 1e-15);
    CHECK_NEAR(evaluate_text("2*pi", 0.0), 2.0 * pi, 1e-15);
    CHECK_NEAR(evaluate_text("sin(pi/2)", 0.0), 1.0, 1e-14);

    CHECK_NEAR(evaluate_text("sin(0.3)", 0.0), sin(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("cos(0.3)", 0.0), cos(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("tan(0.3)", 0.0), tan(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("asin(0.3)", 0.0), asin(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("acos(0.3)", 0.0), acos(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("atan(0.3)", 0.0), atan(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("sinh(0.3)", 0.0), sinh(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("cosh(0.3)", 0.0), cosh(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("tanh(0.3)", 0.0), tanh(0.3), 1e-14);
    CHECK_NEAR(evaluate_text("sqrt(9)", 0.0), 3.0, 1e-14);
    CHECK_NEAR(evaluate_text("abs(-3)", 0.0), 3.0, 1e-14);
    CHECK_NEAR(evaluate_text("exp(1)", 0.0), exp(1.0), 1e-14);
    CHECK_NEAR(evaluate_text("log(e)", 0.0), 1.0, 1e-14);
    CHECK_NEAR(evaluate_text("log10(100)", 0.0), 2.0, 1e-14);
    CHECK_NEAR(evaluate_text("floor(2.9)", 0.0), 2.0, 1e-14);
    CHECK_NEAR(evaluate_text("ceil(2.1)", 0.0), 3.0, 1e-14);
    CHECK_NEAR(evaluate_text("pow(2, 3)", 0.0), 8.0, 1e-14);
    CHECK_NEAR(evaluate_text("min(2, 3)", 0.0), 2.0, 1e-14);
    CHECK_NEAR(evaluate_text("max(2, 3)", 0.0), 3.0, 1e-14);

    CHECK_NEAR(evaluate_text("sqrt(x^2 + 1)", 2.0), sqrt(5.0), 1e-14);
    CHECK_NEAR(evaluate_text("sin(cos(x))", 0.4), sin(cos(0.4)), 1e-14);
    CHECK_NEAR(evaluate_text("log(exp(x))", 1.2), 1.2, 1e-14);
    CHECK_NEAR(evaluate_text("pow(x + 1, 2)", 2.0), 9.0, 1e-14);
}

static void check_compile_error(const char *source)
{
    numerix_expression *expression = (numerix_expression *)1;
    numerix_error error;
    numerix_status status;

    status = numerix_expression_compile(source, &expression, &error);

    CHECK(status != NUMERIX_STATUS_SUCCESS);
    CHECK(expression == NULL);
    CHECK(error.message[0] != '\0');
}

static void test_syntax_errors(void)
{
    check_compile_error("");
    check_compile_error("   \t");
    check_compile_error("x +");
    check_compile_error("* x");
    check_compile_error("sin(");
    check_compile_error("x + )");
    check_compile_error("pow(x)");
    check_compile_error("sqrt(x, 2)");
    check_compile_error("x ** 2");
    check_compile_error("(x + 1");
    check_compile_error("x + 1)");
    check_compile_error("min(x,)");
    check_compile_error("x, 2");
    check_compile_error("y + 1");
    check_compile_error("unknown(x)");
    check_compile_error("x $ 2");
    check_compile_error("1e+");
    check_compile_error("2x");

    {
        numerix_expression *expression = NULL;
        numerix_error error;

        CHECK(numerix_expression_compile("x + @", &expression, &error) ==
              NUMERIX_STATUS_INVALID_INPUT);
        CHECK(strstr(error.message, "character") != NULL);
        CHECK(numerix_expression_compile(NULL, &expression, &error) ==
              NUMERIX_STATUS_INVALID_ARGUMENT);
        CHECK(numerix_expression_compile("x", NULL, &error) ==
              NUMERIX_STATUS_INVALID_ARGUMENT);
    }
}

static numerix_status evaluate_status(
    const char *source,
    double x,
    double *value,
    numerix_error *error)
{
    numerix_expression *expression = NULL;
    numerix_status status = numerix_expression_compile(source, &expression, error);

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = numerix_expression_evaluate(expression, x, value, error);
    }

    numerix_expression_destroy(expression);
    return status;
}

static void test_evaluation_errors(void)
{
    numerix_error error;
    double value;

    CHECK(evaluate_status("1 / (x - 2)", 2.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(strstr(error.message, "division by zero") != NULL);
    CHECK(evaluate_status("sqrt(-1)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("log(0)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("log(-1)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("log10(0)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("asin(2)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("acos(-2)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("pow(-2, 0.5)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("pow(0, -1)", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(evaluate_status("exp(10000)", 0.0, &value, &error) ==
          NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(evaluate_status("1e999", 0.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);

    {
        numerix_expression *expression = NULL;

        CHECK(numerix_expression_compile("x", &expression, &error) ==
              NUMERIX_STATUS_SUCCESS);
        CHECK(numerix_expression_evaluate(expression, INFINITY, &value, &error) ==
              NUMERIX_STATUS_INVALID_ARGUMENT);
        CHECK(numerix_expression_evaluate(NULL, 1.0, &value, &error) ==
              NUMERIX_STATUS_INVALID_ARGUMENT);
        CHECK(numerix_expression_evaluate(expression, 1.0, NULL, &error) ==
              NUMERIX_STATUS_INVALID_ARGUMENT);
        numerix_expression_destroy(expression);
        numerix_expression_destroy(NULL);
    }
}

static void test_callback_and_numerical_algorithms(void)
{
    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;
    numerix_error error;
    numerix_options options;
    numerix_result result;
    double callback_value;
    numerix_status status;

    status = numerix_expression_compile("x^2 - 4", &expression, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_expression_callback_context_init(
              &callback_context, expression, &error) == NUMERIX_STATUS_SUCCESS);
    callback_value = numerix_expression_callback(3.0, &callback_context);
    CHECK_NEAR(callback_value, 5.0, 1e-14);
    CHECK(numerix_expression_callback_status(&callback_context) ==
          NUMERIX_STATUS_SUCCESS);

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-10;
    options.relative_tolerance = 1e-10;
    status = numerix_root_bisection(
        numerix_expression_callback,
        &callback_context,
        0.0,
        3.0,
        &options,
        &result,
        &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK_NEAR(result.value, 2.0, 1e-8);
    CHECK(numerix_expression_callback_status(&callback_context) ==
          NUMERIX_STATUS_SUCCESS);
    numerix_expression_destroy(expression);

    expression = NULL;
    status = numerix_expression_compile("sin(x)", &expression, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_expression_callback_context_init(
              &callback_context, expression, &error) == NUMERIX_STATUS_SUCCESS);
    status = numerix_integrate_adaptive_simpson(
        numerix_expression_callback,
        &callback_context,
        0.0,
        3.14159265358979323846,
        &options,
        24,
        &result,
        &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK_NEAR(result.value, 2.0, 1e-8);
    CHECK(numerix_expression_callback_status(&callback_context) ==
          NUMERIX_STATUS_SUCCESS);
    numerix_expression_destroy(expression);

    expression = NULL;
    status = numerix_expression_compile("x^3", &expression, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_expression_callback_context_init(
              &callback_context, expression, &error) == NUMERIX_STATUS_SUCCESS);
    status = numerix_diff_central(
        numerix_expression_callback,
        &callback_context,
        2.0,
        1e-4,
        &options,
        &result,
        &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK_NEAR(result.value, 12.0, 1e-7);
    numerix_expression_destroy(expression);

    expression = NULL;
    CHECK(numerix_expression_compile("1 / (x - 1)", &expression, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_expression_callback_context_init(
              &callback_context, expression, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_root_bisection(
              numerix_expression_callback,
              &callback_context,
              0.0,
              2.0,
              &options,
              &result,
              &error) == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(numerix_expression_callback_status(&callback_context) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(strstr(
              numerix_expression_callback_error(&callback_context)->message,
              "division by zero") != NULL);
    numerix_expression_destroy(expression);
}

static void test_repeated_lifetimes(void)
{
    size_t iteration;

    for (iteration = 0; iteration < 1000; ++iteration)
    {
        numerix_expression *expression = NULL;
        numerix_error error;
        double value;

        CHECK(numerix_expression_compile(
                  "max(sqrt(x^2 + 1), pow(2, 3))",
                  &expression,
                  &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(numerix_expression_evaluate(expression, 2.0, &value, &error) ==
              NUMERIX_STATUS_SUCCESS);
        CHECK(value == 8.0);
        numerix_expression_destroy(expression);
    }
}

int main(void)
{
    test_numeric_literals_and_arithmetic();
    test_precedence_and_variables();
    test_constants_and_functions();
    test_syntax_errors();
    test_evaluation_errors();
    test_callback_and_numerical_algorithms();
    test_repeated_lifetimes();

    if (checks_failed != 0)
    {
        fprintf(stderr, "%lu of %lu expression checks failed.\n",
                (unsigned long)checks_failed,
                (unsigned long)checks_run);
        return EXIT_FAILURE;
    }

    printf("All %lu expression checks passed.\n", (unsigned long)checks_run);
    return EXIT_SUCCESS;
}
