#include "numerix.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
    ++failures; } } while (0)

static double scaled(double x, void *data)
{ return x * *(const double *)data; }

static double nonfinite_callback(double x, void *data)
{ (void)x; (void)data; return NAN; }

static void test_core(void)
{
    numerix_options options;
    numerix_error error;
    CHECK(NUMERIX_VERSION_MAJOR == 0);
    CHECK(strcmp(numerix_status_string(NUMERIX_STATUS_SUCCESS), "success") == 0);
    CHECK(strcmp(numerix_status_string((numerix_status)999), "unknown status") == 0);
    numerix_options_init(&options);
    CHECK(options.absolute_tolerance > 0.0);
    CHECK(options.relative_tolerance > 0.0);
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_SUCCESS);
    options.absolute_tolerance = -1.0;
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_INVALID_INPUT);
    CHECK(error.code == NUMERIX_STATUS_INVALID_INPUT && error.message[0] != '\0');
    options.absolute_tolerance = 1e-8;
    options.maximum_iterations = 0;
    CHECK(numerix_options_validate(&options, &error) == NUMERIX_STATUS_INVALID_INPUT);
    numerix_error_clear(&error);
    CHECK(error.code == NUMERIX_STATUS_SUCCESS && error.message[0] == '\0');
    CHECK(numerix_error_set(&error, NUMERIX_STATUS_INTERNAL_ERROR, "custom") ==
          NUMERIX_STATUS_INTERNAL_ERROR);
    CHECK(strcmp(error.message, "custom") == 0);
    CHECK(numerix_error_set(NULL, NUMERIX_STATUS_SUCCESS, NULL) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
}

static void test_callback_and_memory(void)
{
    numerix_scalar_problem problem;
    numerix_error error;
    double scale = 3.0, value = 0.0;
    void *memory = numerix_allocate(32);
    CHECK(memory != NULL);
    numerix_deallocate(memory);
    numerix_deallocate(NULL);
    CHECK(numerix_allocate(0) == NULL);
    CHECK(numerix_scalar_problem_init(&problem, scaled, &scale, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_scalar_problem_evaluate(&problem, 2.0, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(value == 6.0);
    CHECK(numerix_scalar_problem_init(&problem, NULL, NULL, &error) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
    problem.function = NULL;
    CHECK(numerix_scalar_problem_evaluate(&problem, 2.0, &value, &error) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
    CHECK(numerix_scalar_problem_init(&problem, nonfinite_callback, NULL, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_scalar_problem_evaluate(&problem, 2.0, &value, &error) ==
          NUMERIX_STATUS_NUMERICAL_FAILURE);
}

static void test_context_and_integration(void)
{
    numerix_options options;
    numerix_error error;
    numerix_context *context = NULL;
    numerix_scalar_problem problem;
    double scale = 2.5, value;
    numerix_options_init(&options);
    CHECK(numerix_context_create(&options, &context, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(context != NULL);
    CHECK(numerix_context_record_iteration(context) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_context_record_function_evaluation(context) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_context_iteration_count(context) == 1);
    CHECK(numerix_context_function_evaluation_count(context) == 1);
    CHECK(numerix_context_elapsed_seconds(context) >= 0.0);
    CHECK(numerix_scalar_problem_init(&problem, scaled, &scale, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_scalar_problem_evaluate(&problem, 4.0, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(value == 10.0);
    CHECK(numerix_context_record_function_evaluation(context) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_context_reset_counters(context) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_context_iteration_count(context) == 0);
    CHECK(numerix_context_function_evaluation_count(context) == 0);
    numerix_context_destroy(context);
    numerix_context_destroy(NULL);
    context = (numerix_context *)1;
    options.relative_tolerance = -1.0;
    CHECK(numerix_context_create(&options, &context, &error) == NUMERIX_STATUS_INVALID_INPUT);
    CHECK(context == NULL);
    CHECK(numerix_context_create(NULL, NULL, &error) == NUMERIX_STATUS_INVALID_ARGUMENT);
}

int main(int argc, char **argv)
{
    numerix_error error;
    int integration_only = argc > 1 && strcmp(argv[1], "integration") == 0;
    if (!integration_only) {
        test_core();
        test_callback_and_memory();
    }
    if (integration_only || argc == 1 || strcmp(argv[1], "unit") == 0)
        test_context_and_integration();
    CHECK(numerix_initialize(&error) == NUMERIX_STATUS_SUCCESS);
    if (failures != 0) {
        fprintf(stderr, "%d test assertion(s) failed.\n", failures);
        return 1;
    }
    puts("All NUMERIX Phase 0 tests passed.");
    return 0;
}
