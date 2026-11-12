#include "numerix.h"

#include <math.h>
#include <stdio.h>

/* f(x) = x^2 - 4. On [3, 4], f(3)=5 and f(4)=12: same sign, no bracket. */
static double f(double x, void *user_data)
{
    (void)user_data;
    return x * x - 4.0;
}

int main(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);

    /* Deliberately invalid bracket: no sign change on [3, 4]. */
    status = numerix_root_brent(f, NULL, 3.0, 4.0, &options, &result, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Unexpected success: root = %.10f\n", result.value);
        return 1;
    }

    printf("Status:     %s\n", numerix_status_string(status));
    printf("Error code: %d\n", (int)error.code);
    printf("Message:    %s\n", error.message);

    /* Correct handling of failure: no owned resources to clean up here
       because Brent's method does not allocate on failure, but the pattern
       of checking every status before using results is essential. */
    puts("Failure handled correctly: no root was produced, no resources leaked.");

    /* Explain why status checking matters in numerical software: */
    puts("A silent failure would have printed a garbage root value instead of");
    puts("rejecting the invalid bracket. Always check the returned status.");

    return 0;
}
