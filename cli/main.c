#include "numerix.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
    printf("Usage: %s [--help | --demo | --eval EXPR X | --root EXPR LOWER UPPER |\n"
           "       --roots EXPR LOWER UPPER | --piecewise-demo]\n", program);
    puts("Quote expressions containing spaces or shell metacharacters.");
    puts("--root shows recommendation, managed execution and residual diagnostics.");
    puts("--roots performs heuristic discovery; completeness is never guaranteed.");
}

static int parse_number(const char *text, double *value)
{
    char *end;

    errno = 0;
    *value = strtod(text, &end);
    return end != text && *end == '\0' && errno == 0 && isfinite(*value);
}

static int evaluate_expression(const char *source, double x)
{
    numerix_expression *expression = NULL;
    numerix_error error;
    numerix_status status;
    double value;

    status = numerix_expression_compile(source, &expression, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_expression_evaluate(expression, x, &value, &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("f(%.12g) = %.12g\n", x, value);
    } else {
        fprintf(stderr, "Expression failed: %s\n", error.message);
    }
    numerix_expression_destroy(expression);
    return status == NUMERIX_STATUS_SUCCESS ? 0 : 1;
}

static int solve_expression(const char *source, double lower, double upper,
                            int discover)
{
    numerix_expression *expression = NULL;
    numerix_recommendation_request request;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    status = numerix_expression_compile(source, &expression, &error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Expression failed: %s\n", error.message);
        return 1;
    }
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_EXPRESSION;
    request.expression = expression;
    request.problem.root.lower = lower;
    request.problem.root.upper = upper;
    request.problem.root.has_bracket = 1;
    if (discover) {
        numerix_root_search_report report;
        size_t index;

        status = numerix_root_find_all(&request, lower, upper, NULL, &report, &error);
        printf("Discovery: %s; %zu roots; %zu unresolved intervals\n",
               numerix_status_string(status), report.root_count, report.unresolved_count);
        for (index = 0; index < report.root_count; ++index) {
            printf("  x = %.12g", report.roots[index].result.value);
            if (isfinite(report.roots[index].result.residual)) {
                printf(", residual = %.3e", report.roots[index].result.residual);
            } else {
                printf(", residual unavailable");
            }
            printf("\n");
        }
        puts("Heuristic search: finding no roots does not prove that none exist.");
        numerix_root_search_report_destroy(&report);
    } else {
        numerix_managed_execution *execution = NULL;
        numerix_execution_summary summary;
        const numerix_execution_output *output = NULL;
        numerix_diagnostics_report diagnostics;

        status = numerix_managed_execution_create(&request, NULL, &execution, &error);
        if (status == NUMERIX_STATUS_SUCCESS) {
            status = numerix_managed_execution_run(execution, &error);
        }
        if (status == NUMERIX_STATUS_SUCCESS) {
            status = numerix_managed_execution_get_summary(execution, &summary, &error);
        }
        if (status == NUMERIX_STATUS_SUCCESS) {
            printf("Selected: %s\n", numerix_algorithm_name(summary.final_algorithm));
            printf("Root = %.12g; ", summary.result.value);
            if (isfinite(summary.result.residual)) {
                printf("residual = %.3e; ", summary.result.residual);
            } else {
                printf("residual unavailable; ");
            }
            printf("%zu iterations; %zu evaluations\n", summary.iterations,
                   summary.function_evaluations);
            printf("Managed execution state = %d; elapsed = %.6g s\n",
                   (int)summary.state, summary.elapsed_seconds);
            status = numerix_managed_execution_get_output(execution, &output, &error);
        }
        if (status == NUMERIX_STATUS_SUCCESS) {
            printf("Recommendation: %s\n", output->recommendation.reason[0]);
            status = numerix_diagnostics_analyze(&request, output, NULL,
                                                &diagnostics, &error);
            if (status == NUMERIX_STATUS_SUCCESS) {
                if (diagnostics.has_residual) {
                    printf("Diagnostic residual = %.3e; ",
                           diagnostics.absolute_residual);
                } else {
                    printf("Diagnostic residual unavailable; ");
                }
                printf("reliability = %s\n",
                       numerix_reliability_class_name(diagnostics.reliability));
                numerix_diagnostics_report_destroy(&diagnostics);
            }
        }
        numerix_managed_execution_destroy(execution);
    }
    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Numerical operation failed: %s\n", error.message);
    }
    numerix_expression_destroy(expression);
    return status == NUMERIX_STATUS_SUCCESS ? 0 : 1;
}

static int demonstrate_piecewise(void)
{
    numerix_piecewise_function *function = NULL;
    numerix_piecewise_boundary_report report;
    numerix_error error;
    numerix_status status;
    double value;

    status = numerix_piecewise_create(&function, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(function, -1.0, 0.0,
                                                   1, 0, "x", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(function, 0.0, 1.0,
                                                   1, 1, "x+1", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_evaluate(function, 0.0, &value, &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("Piecewise x for x<0, x+1 for x>=0: f(0) = %.12g\n", value);
        status = numerix_piecewise_analyze_boundary(function, 0.0, NULL, &report, &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("Boundary: left limit %.6g, right limit %.6g, classification %d\n",
               report.left_limit, report.right_limit, (int)report.type);
    } else {
        fprintf(stderr, "Piecewise demo failed: %s\n", error.message);
    }
    numerix_piecewise_destroy(function);
    return status == NUMERIX_STATUS_SUCCESS ? 0 : 1;
}

int main(int argc, char **argv)
{
    numerix_error error;
    double lower;
    double upper;
    numerix_status status = numerix_initialize(&error);

    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Initialization failed: %s\n", error.message);
        return 1;
    }
    if (argc == 1) {
        printf("NUMERIX %s\n", NUMERIX_VERSION_STRING);
        puts("Intelligent numerical computation library in C11.");
        puts("Core library initialized successfully. Use --help for demonstrations.");
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        return solve_expression("x^2-2", 0.0, 2.0, 0);
    }
    if (argc == 2 && strcmp(argv[1], "--piecewise-demo") == 0) {
        return demonstrate_piecewise();
    }
    if (argc == 4 && strcmp(argv[1], "--eval") == 0 &&
        parse_number(argv[3], &lower)) {
        return evaluate_expression(argv[2], lower);
    }
    if (argc == 5 && (strcmp(argv[1], "--root") == 0 ||
                      strcmp(argv[1], "--roots") == 0) &&
        parse_number(argv[3], &lower) && parse_number(argv[4], &upper) && lower < upper) {
        return solve_expression(argv[2], lower, upper, strcmp(argv[1], "--roots") == 0);
    }
    fputs("Invalid arguments: expected finite numbers and ordered bounds.\n", stderr);
    print_usage(argv[0]);
    return 2;
}
