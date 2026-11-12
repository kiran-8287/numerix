#include "numerix.h"

#include <math.h>
#include <stdio.h>

static double f(double x, void *ud)
{
    (void)ud;
    return x * x - 4.0;
}

int main(void)
{
    numerix_error error;
    numerix_status status;

    printf("NUMERIX version: %s\n", NUMERIX_VERSION_STRING);

    status = numerix_initialize(&error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "numerix_initialize failed: %s\n", error.message);
        return 1;
    }
    puts("numerix_initialize: success");

    numerix_options options;
    numerix_result result;
    numerix_error err;
    double root;

    numerix_options_init(&options);
    status = numerix_root_brent(f, NULL, 0.0, 3.0, &options, &result, &err);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "root solve failed: %s\n", err.message);
        return 1;
    }
    root = result.value;
    printf("Brent root of x^2 - 4 on [0, 3]: %.10f (expected 2.0000000000)\n", root);

    return 0;
}
