#include "numerix.h"
#include <stdio.h>

int main(void)
{
    numerix_error error;
    numerix_status status = numerix_initialize(&error);
    printf("NUMERIX %s\n", NUMERIX_VERSION_STRING);
    puts("Intelligent numerical computation library in C11.");
    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Initialization failed: %s\n", error.message);
        return 1;
    }
    puts("Core library initialized successfully.");
    return 0;
}
