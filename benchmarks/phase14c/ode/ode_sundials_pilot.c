#include "numerix.h"

#include <arkode/arkode.h>
#include <arkode/arkode_butcher_erk.h>
#include <arkode/arkode_erkstep.h>
#include <math.h>
#include <nvector/nvector_serial.h>
#include <stdio.h>
#include <string.h>
#include <sundials/sundials_context.h>

#define DIMENSION 2U
#define STEP_COUNT 20U
#define POINT_COUNT (STEP_COUNT + 1U)
#define INITIAL_TIME 0.0
#define STEP_SIZE 0.1

typedef enum pilot_method {
    PILOT_EULER = 0,
    PILOT_HEUN,
    PILOT_RK4
} pilot_method;

typedef struct pilot_system {
    double lambda[DIMENSION];
    size_t callback_count;
} pilot_system;

static const char *method_name(pilot_method method)
{
    switch (method) {
    case PILOT_EULER: return "Euler";
    case PILOT_HEUN: return "Heun";
    case PILOT_RK4: return "classical RK4";
    }
    return "unknown";
}

static double max_abs_trajectory_error(const double *trajectory,
                                       const double *lambda,
                                       size_t points)
{
    size_t point;
    size_t component;
    double maximum = 0.0;
    for (point = 0; point < points; ++point) {
        const double time = INITIAL_TIME + (double)point * STEP_SIZE;
        for (component = 0; component < DIMENSION; ++component) {
            const double reference = exp(lambda[component] * time);
            const double value = trajectory[point * DIMENSION + component];
            maximum = fmax(maximum, fabs(value - reference));
        }
    }
    return maximum;
}

static numerix_status numerix_rhs(double time, const double *state,
                                  size_t dimension, double *derivative,
                                  void *user_data)
{
    pilot_system *system = (pilot_system *)user_data;
    size_t component;
    (void)time;
    if (dimension != DIMENSION) return NUMERIX_STATUS_INVALID_DIMENSIONS;
    ++system->callback_count;
    for (component = 0; component < dimension; ++component) {
        derivative[component] = system->lambda[component] * state[component];
    }
    return NUMERIX_STATUS_SUCCESS;
}

static int sundials_rhs(sunrealtype time, N_Vector state,
                        N_Vector derivative, void *user_data)
{
    pilot_system *system = (pilot_system *)user_data;
    const sunrealtype *y = N_VGetArrayPointer(state);
    sunrealtype *dydt = N_VGetArrayPointer(derivative);
    size_t component;
    (void)time;
    ++system->callback_count;
    for (component = 0; component < DIMENSION; ++component) {
        dydt[component] = (sunrealtype)system->lambda[component] * y[component];
    }
    return 0;
}

static int select_method(pilot_method method, ARKODE_ERKTableID *table_id)
{
    switch (method) {
    case PILOT_EULER:
        *table_id = ARKODE_FORWARD_EULER_1_1;
        return 1;
    case PILOT_HEUN:
        *table_id = ARKODE_HEUN_EULER_2_1_2;
        return 1;
    case PILOT_RK4:
        *table_id = ARKODE_KUTTA_RK4a_4_4;
        return 1;
    }
    return 0;
}

static int table_coefficients_match(pilot_method method,
                                    ARKODE_ERKTableID table_id)
{
    ARKodeButcherTable table = ARKodeButcherTable_LoadERK(table_id);
    int matches = 0;
    if (table == NULL) return 0;
    if (method == PILOT_EULER) {
        matches = table->stages == 1 && fabs((double)table->b[0] - 1.0) < 1e-15;
    } else if (method == PILOT_HEUN) {
        matches = table->stages == 2 &&
            fabs((double)table->A[1][0] - 1.0) < 1e-15 &&
            fabs((double)table->b[0] - 0.5) < 1e-15 &&
            fabs((double)table->b[1] - 0.5) < 1e-15 &&
            fabs((double)table->c[1] - 1.0) < 1e-15;
    } else if (method == PILOT_RK4) {
        matches = table->stages == 4 &&
            fabs((double)table->A[1][0] - 0.5) < 1e-15 &&
            fabs((double)table->A[2][1] - 0.5) < 1e-15 &&
            fabs((double)table->A[3][2] - 1.0) < 1e-15 &&
            fabs((double)table->b[0] - 1.0 / 6.0) < 1e-15 &&
            fabs((double)table->b[1] - 1.0 / 3.0) < 1e-15 &&
            fabs((double)table->b[2] - 1.0 / 3.0) < 1e-15 &&
            fabs((double)table->b[3] - 1.0 / 6.0) < 1e-15 &&
            fabs((double)table->c[1] - 0.5) < 1e-15 &&
            fabs((double)table->c[2] - 0.5) < 1e-15 &&
            fabs((double)table->c[3] - 1.0) < 1e-15;
    }
    ARKodeButcherTable_Free(table);
    return matches;
}

static int run_numerix(pilot_method method, const double *initial_state,
                       pilot_system *system, double *trajectory,
                       size_t *rhs_evaluations)
{
    numerix_ode_solution solution;
    numerix_error error;
    numerix_status status;
    memset(&solution, 0, sizeof(solution));
    system->callback_count = 0;
    if (method == PILOT_EULER) {
        status = numerix_ode_euler(numerix_rhs, system, INITIAL_TIME,
            initial_state, DIMENSION, STEP_SIZE, STEP_COUNT, &solution, &error);
    } else if (method == PILOT_HEUN) {
        status = numerix_ode_heun(numerix_rhs, system, INITIAL_TIME,
            initial_state, DIMENSION, STEP_SIZE, STEP_COUNT, &solution, &error);
    } else {
        status = numerix_ode_rk4(numerix_rhs, system, INITIAL_TIME,
            initial_state, DIMENSION, STEP_SIZE, STEP_COUNT, &solution, &error);
    }
    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "NUMERIX %s failed: %s\n", method_name(method), error.message);
        numerix_ode_solution_destroy(&solution);
        return 0;
    }
    if (solution.points != POINT_COUNT || solution.dimension != DIMENSION) {
        fprintf(stderr, "NUMERIX %s returned an unexpected trajectory shape\n",
                method_name(method));
        numerix_ode_solution_destroy(&solution);
        return 0;
    }
    memcpy(trajectory, solution.y, POINT_COUNT * DIMENSION * sizeof(double));
    *rhs_evaluations = solution.function_evaluations;
    numerix_ode_solution_destroy(&solution);
    return 1;
}

static int run_sundials(pilot_method method, const double *initial_state,
                        pilot_system *system, double *trajectory,
                        size_t *rhs_evaluations)
{
    SUNContext context = NULL;
    N_Vector state = NULL;
    void *memory = NULL;
    ARKODE_ERKTableID table_id;
    int status = -1;
    size_t point;
    long int evaluations = 0;
    sunrealtype *y;

    if (!select_method(method, &table_id) || !table_coefficients_match(method, table_id)) {
        fprintf(stderr, "SUNDIALS %s Butcher tableau did not match NUMERIX\n",
                method_name(method));
        return 0;
    }
    if (SUNContext_Create(SUN_COMM_NULL, &context) != SUN_SUCCESS) goto cleanup;
    state = N_VNew_Serial((sunindextype)DIMENSION, context);
    if (state == NULL) goto cleanup;
    y = N_VGetArrayPointer(state);
    for (point = 0; point < DIMENSION; ++point) y[point] = (sunrealtype)initial_state[point];
    memory = ERKStepCreate(sundials_rhs, (sunrealtype)INITIAL_TIME, state, context);
    if (memory == NULL) goto cleanup;
    status = ERKStepSetTableNum(memory, table_id);
    if (status < 0) goto cleanup;
    status = ARKodeSetUserData(memory, system);
    if (status < 0) goto cleanup;
    status = ARKodeSetFixedStep(memory, (sunrealtype)STEP_SIZE);
    if (status < 0) goto cleanup;
    status = ARKodeSetMaxNumSteps(memory, (long int)(STEP_COUNT + 2U));
    if (status < 0) goto cleanup;
    trajectory[0] = (double)y[0];
    trajectory[1] = (double)y[1];
    system->callback_count = 0;
    for (point = 1; point < POINT_COUNT; ++point) {
        sunrealtype reached = (sunrealtype)INITIAL_TIME;
        const sunrealtype target = (sunrealtype)((double)point * STEP_SIZE);
        status = ARKodeEvolve(memory, target, state, &reached, ARK_NORMAL);
        if (status < 0) goto cleanup;
        if (fabs((double)(reached - target)) > 1e-13) {
            status = -1;
            goto cleanup;
        }
        y = N_VGetArrayPointer(state);
        trajectory[point * DIMENSION] = (double)y[0];
        trajectory[point * DIMENSION + 1U] = (double)y[1];
    }
    status = ARKodeGetNumRhsEvals(memory, 0, &evaluations);
    if (status < 0) goto cleanup;
    *rhs_evaluations = (size_t)evaluations;
    status = 0;

cleanup:
    if (memory != NULL) ARKodeFree(&memory);
    if (state != NULL) N_VDestroy(state);
    if (context != NULL) SUNContext_Free(&context);
    if (status < 0) {
        fprintf(stderr, "SUNDIALS %s pilot failed (native status %d)\n",
                method_name(method), status);
        return 0;
    }
    return 1;
}

int main(void)
{
    const double initial_state[DIMENSION] = {1.0, 1.0};
    const double lambda[DIMENSION] = {-0.25, 1.0};
    const pilot_method methods[] = {PILOT_EULER, PILOT_HEUN, PILOT_RK4};
    size_t index;
    printf("ODE table-equivalence pilot; not a benchmark measurement\n");
    printf("Problem: y'_j = lambda_j y_j; y_j(t)=exp(lambda_j t), 2 states, %zu fixed steps\n",
           (size_t)STEP_COUNT);
    for (index = 0; index < sizeof(methods) / sizeof(methods[0]); ++index) {
        const pilot_method method = methods[index];
        pilot_system numerix_system = {{lambda[0], lambda[1]}, 0U};
        pilot_system sundials_system = {{lambda[0], lambda[1]}, 0U};
        double numerix_trajectory[POINT_COUNT * DIMENSION];
        double sundials_trajectory[POINT_COUNT * DIMENSION];
        size_t numerix_evaluations = 0U;
        size_t sundials_evaluations = 0U;
        double max_difference = 0.0;
        size_t element;
        if (!run_numerix(method, initial_state, &numerix_system,
                         numerix_trajectory, &numerix_evaluations) ||
            !run_sundials(method, initial_state, &sundials_system,
                          sundials_trajectory, &sundials_evaluations)) return 1;
        for (element = 0; element < POINT_COUNT * DIMENSION; ++element) {
            max_difference = fmax(max_difference,
                fabs(numerix_trajectory[element] - sundials_trajectory[element]));
        }
        printf("%s: tableau=matched; points=%u; NUMERIX_rhs=%zu; SUNDIALS_rhs=%zu; "
               "max_abs_traj_error NUMERIX=%.9e SUNDIALS=%.9e; cross-library_delta=%.9e\n",
               method_name(method), (unsigned int)POINT_COUNT,
               numerix_evaluations, sundials_evaluations,
               max_abs_trajectory_error(numerix_trajectory, lambda, POINT_COUNT),
               max_abs_trajectory_error(sundials_trajectory, lambda, POINT_COUNT),
               max_difference);
    }
    return 0;
}
