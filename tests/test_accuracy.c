#include "numerix.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static size_t failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

typedef struct root_case
{
    double scale;
    int power;
    int exponential;
} root_case;

static double root_function(double position, void *data)
{
    const root_case *definition = data;

    if (definition->exponential)
    {
        return definition->scale * (exp(position) - 3.0);
    }
    return definition->scale * pow(position - 2.0, (double)definition->power);
}

static double root_derivative(double position, void *data)
{
    const root_case *definition = data;

    if (definition->exponential)
    {
        return definition->scale * exp(position);
    }
    return definition->scale * (double)definition->power *
        pow(position - 2.0, (double)(definition->power - 1));
}

static numerix_status run_root(int method, root_case *definition,
                               const numerix_options *options, numerix_result *result)
{
    numerix_error error;

    switch (method)
    {
        case 0:
            return numerix_root_bisection(root_function, definition, 0.5, 4.5, options, result, &error);
        case 1:
            return numerix_root_regula_falsi(root_function, definition, 0.5, 4.5, options, result, &error);
        case 2:
            return numerix_root_brent(root_function, definition, 0.5, 4.5, options, result, &error);
        case 3:
            return numerix_root_newton(root_function, root_derivative, definition, 3.7, options, result, &error);
        case 4:
            return numerix_root_safeguarded_newton(root_function, root_derivative,
                definition, 0.5, 4.5, options, result, &error);
        default:
            return numerix_root_secant(root_function, definition, 0.5, 3.7, options, result, &error);
    }
}

static double plateau(double position, void *data)
{
    (void)position;
    (void)data;
    return 1.0;
}

static double excessive_derivative(double position, void *data)
{
    (void)position;
    (void)data;
    return 1e100;
}

static double endpoint_function(double position, void *data)
{
    (void)data;
    return position - 1e-12;
}

static double jump_function(double position, void *data)
{
    (void)data;
    return position < 0.3 ? -1.0 : 1.0;
}

static void test_root_accuracy(void)
{
    root_case cases[] = {
        {1.0, 1, 0}, {1e-12, 1, 0}, {1e12, 1, 0}, {1e-200, 1, 0},
        {1e200, 1, 0}, {1.0, 3, 0}, {1e-12, 3, 0}, {1.0, 9, 0},
        {1.0, 1, 1}, {1e-200, 1, 1}, {1e200, 1, 1}
    };
    size_t index;
    int method;
    numerix_options options;
    numerix_result result;
    numerix_error error;

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-10;
    options.relative_tolerance = 1e-10;
    options.maximum_iterations = 2000U;
    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        for (method = 0; method < 6; ++method)
        {
            double reference = cases[index].exponential ? log(3.0) : 2.0;
            double tolerance = options.absolute_tolerance + options.relative_tolerance * fabs(reference);
            numerix_status status = run_root(method, &cases[index], &options, &result);

            if (status != NUMERIX_STATUS_SUCCESS || fabs(result.value - reference) > tolerance)
            {
                fprintf(stderr, "root case %zu method %d status %d value %.17g reference %.17g\n",
                        index, method, (int)status, result.value, reference);
            }
            CHECK(status == NUMERIX_STATUS_SUCCESS);
            CHECK(fabs(result.value - reference) <= tolerance);
            if (isfinite(result.residual))
            {
                CHECK(fabs(result.residual / cases[index].scale) <=
                      2e-8 * fmax(1.0, fabs(root_derivative(reference, &cases[index]) / cases[index].scale)));
            }
            else
            {
                /* Bisection may finish from its certified bracket-width bound. */
                CHECK(method == 0 && isnan(result.residual));
            }
            CHECK(result.estimated_error <= options.absolute_tolerance +
                  options.relative_tolerance * fabs(result.value));
        }
    }
    options.absolute_tolerance = 1e-15;
    options.relative_tolerance = 0.0;
    CHECK(numerix_root_brent(endpoint_function, NULL, 0.0, 1.0, &options, &result, &error) == 0);
    CHECK(fabs(result.value - 1e-12) <= 1e-15);
    CHECK(numerix_root_newton(plateau, excessive_derivative, NULL, 1.0,
                              &options, &result, &error) == NUMERIX_STATUS_CONVERGENCE_FAILURE);
    CHECK(numerix_root_secant(plateau, NULL, 0.0, 1.0, &options, &result, &error) ==
          NUMERIX_STATUS_NUMERICAL_FAILURE);
    options.maximum_iterations = 1U;
    CHECK(run_root(2, &cases[8], &options, &result) == NUMERIX_STATUS_LIMIT_REACHED);
    options.maximum_iterations = 2000U;
    options.absolute_tolerance = 1e-300;
    CHECK(numerix_root_bisection(jump_function, NULL, 0.0, 1.0, &options, &result, &error) ==
          NUMERIX_STATUS_CONVERGENCE_FAILURE);
}

static double integrand(double position, void *data)
{
    int kind = *(int *)data;

    switch (kind)
    {
        case 0:
            return 2.0;
        case 1:
            return position * position * position * position;
        case 2:
            return sin(position);
        case 3:
            return exp(position);
        case 4:
            return sqrt(position);
        case 5:
            return 1.0 / sqrt(position);
        case 6:
            return sin(40.0 * position);
        default:
            return NAN;
    }
}

static void test_integration_accuracy(void)
{
    const double references[] = {2.0, 0.2, 1.0 - cos(1.0), exp(1.0) - 1.0,
                                  2.0 / 3.0, 2.0, (1.0 - cos(40.0)) / 40.0};
    numerix_options options;
    numerix_result result;
    numerix_error error;
    int kind;

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-10;
    options.relative_tolerance = 1e-10;
    options.maximum_iterations = 100000U;
    options.maximum_function_evaluations = 1000000U;
    for (kind = 0; kind < 7; ++kind)
    {
        numerix_status status = numerix_integrate_adaptive_simpson(integrand, &kind,
            0.0, 1.0, &options, 32U, &result, &error);

        if (kind == 5)
        {
            CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
            CHECK(isnan(result.value));
            continue;
        }
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(fabs(result.value - references[kind]) <=
              fmax(options.absolute_tolerance, options.relative_tolerance * fabs(references[kind])));
        CHECK(result.estimated_error <= fmax(options.absolute_tolerance,
              options.relative_tolerance * fabs(result.value)));
        printf("integrand=%d value=%.17g actual_error=%.9g estimate=%.9g evaluations=%zu\n",
               kind, result.value, fabs(result.value - references[kind]),
               result.estimated_error, result.function_evaluations);
    }
    kind = 4;
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 1.0, 0.0,
          &options, 32U, &result, &error) == 0);
    CHECK(fabs(result.value + 2.0 / 3.0) <= 1e-10);
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 0.0, 1.0,
          &options, 1U, &result, &error) == NUMERIX_STATUS_LIMIT_REACHED);
    options.maximum_function_evaluations = 6U;
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 0.0, 1.0,
          &options, 32U, &result, &error) == NUMERIX_STATUS_LIMIT_REACHED);
    options.maximum_function_evaluations = 1000000U;
    options.maximum_iterations = 1U;
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 0.0, 1.0,
          &options, 32U, &result, &error) == NUMERIX_STATUS_LIMIT_REACHED);
    kind = 7;
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 0.0, 1.0,
          &options, 32U, &result, &error) == NUMERIX_STATUS_NUMERICAL_FAILURE);
    kind = 3;
    options.maximum_iterations = 1000U;
    options.absolute_tolerance = 1e-25;
    options.relative_tolerance = 0.0;
    CHECK(numerix_integrate_adaptive_simpson(integrand, &kind, 0.0, 1.0,
          &options, 32U, &result, &error) != NUMERIX_STATUS_SUCCESS);
}

static void test_workflows_and_options(void)
{
    root_case definition = {1e-12, 1, 0};
    numerix_recommendation_request request;
    numerix_execution_output output;
    numerix_diagnostics_report diagnostics;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_root_search_report discovery;
    numerix_root_discovery_options discovery_options;
    numerix_error error;
    numerix_options options;

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.function = root_function;
    request.derivative = root_derivative;
    request.user_data = &definition;
    request.options = &options;
    request.problem.root.has_bracket = 1;
    request.problem.root.lower = 0.5;
    request.problem.root.upper = 4.5;
    request.problem.root.has_initial = 1;
    request.problem.root.initial = 3.7;
    CHECK(numerix_execute_recommended(&request, NULL, &output, &error) == 0);
    CHECK(fabs(output.scalar_result.value - 2.0) <= 1e-7);
    CHECK(numerix_diagnostics_analyze(&request, &output, NULL, &diagnostics, &error) == 0);
    CHECK(!diagnostics.has_residual_tolerance);
    CHECK(isnan(diagnostics.requested_residual_tolerance));
    numerix_diagnostics_report_destroy(&diagnostics);
    numerix_execution_output_destroy(&output);
    CHECK(numerix_managed_execution_create(&request, NULL, &execution, &error) == 0);
    CHECK(numerix_managed_execution_run(execution, &error) == 0);
    CHECK(numerix_managed_execution_get_summary(execution, &summary, &error) == 0);
    CHECK(fabs(summary.result.value - 2.0) <= 1e-7);
    numerix_managed_execution_destroy(execution);
    numerix_root_discovery_options_init(&discovery_options);
    discovery_options.residual_tolerance = 1e-22;
    CHECK(numerix_root_find_all(&request, 0.5, 4.5, &discovery_options, &discovery, &error) == 0);
    CHECK(discovery.root_count == 1U);
    numerix_root_search_report_destroy(&discovery);
    options.absolute_tolerance = -1.0;
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_INVALID_INPUT);
    options.absolute_tolerance = 0.0;
    options.relative_tolerance = 0.0;
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_INVALID_INPUT);
    options.absolute_tolerance = NAN;
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_INVALID_INPUT);
    options.absolute_tolerance = DBL_MAX;
    CHECK(numerix_options_validate(&options, &error) == 0);
    options.absolute_tolerance = DBL_MIN;
    CHECK(numerix_options_validate(&options, &error) == 0);
}

int main(void)
{
    test_root_accuracy();
    test_integration_accuracy();
    test_workflows_and_options();
    if (failures != 0U)
    {
        fprintf(stderr, "%zu accuracy checks failed\n", failures);
        return 1;
    }
    puts("All accuracy and convergence checks passed.");
    return 0;
}
