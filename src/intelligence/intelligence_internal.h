#ifndef NUMERIX_INTELLIGENCE_INTERNAL_H
#define NUMERIX_INTELLIGENCE_INTERNAL_H

#include "numerix.h"

typedef struct numerix_problem_callback_context {
    const numerix_recommendation_request *request;
    numerix_status last_status;
    numerix_error last_error;
} numerix_problem_callback_context;

int numerix_request_has_function(
    const numerix_recommendation_request *request);
numerix_status numerix_request_evaluate(
    const numerix_recommendation_request *request, double x,
    double *value, numerix_error *error);
numerix_status numerix_problem_callback_context_init(
    numerix_problem_callback_context *context,
    const numerix_recommendation_request *request, numerix_error *error);
double numerix_problem_callback(double x, void *user_data);
double numerix_problem_derivative_callback(double x, void *user_data);

void numerix_recommendation_add_alternative(
    numerix_recommendation *recommendation,
    numerix_algorithm algorithm, const char *reason);
void numerix_recommendation_set(
    numerix_recommendation *recommendation,
    numerix_algorithm algorithm, numerix_suitability suitability,
    const char *reason, const char *limitation);
int numerix_root_algorithm_applicable(
    numerix_algorithm algorithm,
    const numerix_recommendation_request *request,
    int sign_changing_bracket);
numerix_status numerix_root_run_algorithm(
    numerix_algorithm algorithm,
    const numerix_recommendation_request *request,
    numerix_result *result, numerix_error *error);
int numerix_root_result_is_usable(
    const numerix_result *result, const numerix_options *options);

#endif
