#include "numerix.h"

#include <math.h>
#include <stdio.h>

static void print_status(const char *label, numerix_status status,
                         const numerix_error *error)
{
    printf("%s: %s", label, numerix_status_string(status));
    if (status != NUMERIX_STATUS_SUCCESS && error != NULL) {
        printf(" (%s)", error->message);
    }
    putchar('\n');
}

int main(void)
{
    numerix_piecewise_function *function = NULL;
    numerix_piecewise_boundary_report report;
    numerix_result result;
    numerix_error error;
    double value;
    numerix_status status;

    status = numerix_piecewise_create(&function, &error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        print_status("create", status, &error);
        return 1;
    }

    status = numerix_piecewise_add_expression(
        function, -INFINITY, 0.0, 0, 0, "x^2", &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(
            function, 0.0, INFINITY, 1, 0, "2*x", &error);
    }
    if (status != NUMERIX_STATUS_SUCCESS) {
        print_status("add segment", status, &error);
        numerix_piecewise_destroy(function);
        return 1;
    }

    puts("Basic evaluations for x^2 on x < 0 and 2*x on x >= 0:");
    status = numerix_piecewise_evaluate(function, -2.0, &value, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("  f(-2) = %.6f\n", value);
    }
    status = numerix_piecewise_evaluate(function, 0.0, &value, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("  f(0) = %.6f\n", value);
    }

    status = numerix_piecewise_analyze_boundary(
        function, 0.0, NULL, &report, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("Boundary x=0 classification: %d (confidence %d)\n",
               (int)report.type, (int)report.confidence);
    }

    status = numerix_piecewise_integrate_trapezoid(
        function, -2.0, 2.0, 100, NULL, &result, &error);
    print_status("piecewise integral on [-2, 2]", status, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("  integral = %.8f\n", result.value);
    }

    status = numerix_piecewise_diff_central(
        function, 1.0, 1.0e-4, NULL, &result, &error);
    print_status("derivative at x=1", status, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("  derivative = %.8f\n", result.value);
    }

    numerix_piecewise_destroy(function);

    function = NULL;
    status = numerix_piecewise_create(&function, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(
            function, -INFINITY, 0.0, 0, 0, "x", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(
            function, 0.0, INFINITY, 1, 0, "x+2", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_analyze_boundary(
            function, 0.0, NULL, &report, &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("Jump example at x=0: left %.6f, right %.6f, type %d\n",
               report.left_limit, report.right_limit, (int)report.type);
    }
    numerix_piecewise_destroy(function);

    function = NULL;
    status = numerix_piecewise_create(&function, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(
            function, -INFINITY, 1.0, 0, 0, "(x^2-1)/(x-1)", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_add_expression(
            function, 1.0, INFINITY, 0, 0, "(x^2-1)/(x-1)", &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = numerix_piecewise_analyze_boundary(
            function, 1.0, NULL, &report, &error);
    }
    if (status == NUMERIX_STATUS_SUCCESS) {
        printf("Removable example at x=1: limiting value %.6f, type %d\n",
               report.left_limit, (int)report.type);
    }
    numerix_piecewise_destroy(function);
    return 0;
}
