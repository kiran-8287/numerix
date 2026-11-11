#ifndef NUMERIX_EXECUTION_INTERNAL_H
#define NUMERIX_EXECUTION_INTERNAL_H

#include "numerix.h"

typedef numerix_status (*numerix_execution_stop_check)(
    void *user_data, const numerix_recommendation *recommendation);

numerix_status numerix_root_solve_auto_controlled(
    const numerix_recommendation_request *request,
    const numerix_recommendation_request *execution_request,
    const numerix_root_selection_options *selection_options,
    numerix_result *result, numerix_root_selection_report *report,
    numerix_error *error, numerix_execution_stop_check stop_check,
    void *stop_user_data);

#endif
