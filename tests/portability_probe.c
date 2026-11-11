#include "numerix.h"

#include <float.h>
#include <stdio.h>

int main(void)
{
    printf("numerix=%s\n", NUMERIX_VERSION_STRING);
#ifdef _MSC_VER
    printf("compiler=MSVC %d\n", _MSC_VER);
#elif defined(__clang__)
    printf("compiler=Clang %s\n", __clang_version__);
#elif defined(__GNUC__)
    printf("compiler=GCC %s\n", __VERSION__);
#else
    puts("compiler=unidentified");
#endif
    printf("pointer_bytes=%zu\nsize_t_bytes=%zu\n", sizeof(void *), sizeof(size_t));
    printf("double_mantissa=%d\nlong_double_mantissa=%d\n", DBL_MANT_DIG, LDBL_MANT_DIG);
    printf("status_bytes=%zu\nresult_bytes=%zu\n", sizeof(numerix_status), sizeof(numerix_result));
    printf("options_bytes=%zu\ndiagnostics_bytes=%zu\n", sizeof(numerix_options),
           sizeof(numerix_diagnostics_report));
    printf("discovered_root_bytes=%zu\nsearch_report_bytes=%zu\n",
           sizeof(numerix_discovered_root), sizeof(numerix_root_search_report));
    return sizeof(double) >= 8U && DBL_MANT_DIG >= 53 ? 0 : 1;
}
