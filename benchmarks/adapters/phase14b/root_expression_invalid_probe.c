#include "numerix.h"

#include <stdio.h>

int main(void)
{
    numerix_expression *expression = NULL;
    numerix_error error;
    numerix_status status;
    FILE *out;

    numerix_error_clear(&error);
    status = numerix_expression_compile("x + @", &expression, &error);
    out = fopen("benchmarks/results/phase14b/edge_cases/root/expression_invalid_probe.csv", "w");
    if (out == NULL) {
        numerix_expression_destroy(expression);
        return 2;
    }
    fprintf(out, "test_id,expression,status,status_name,error_code,expected_status,passed\n");
    fprintf(out, "root-invalid-expression,\"x + @\",%d,%s,%d,%d,%d\n",
        (int)status, numerix_status_string(status), (int)error.code,
        (int)NUMERIX_STATUS_INVALID_INPUT,
        status == NUMERIX_STATUS_INVALID_INPUT && expression == NULL);
    fclose(out);
    numerix_expression_destroy(expression);
    return status == NUMERIX_STATUS_INVALID_INPUT ? 0 : 1;
}
