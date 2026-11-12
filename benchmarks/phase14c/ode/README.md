# ODE method mapping and ARKODE table pilot

This directory contains an implementation audit and a small correctness pilot
for the NUMERIX fixed-step ODE methods. The pilot is **not benchmark evidence**:
it has one analytic two-state system, does no timing or allocator measurement,
and produces no graph.

## Build and run

The pilot requires the already built SUNDIALS 7.9.0 prefix and the project C11
compiler. For the current machine:

```sh
cmake -S benchmarks/phase14c/ode -B build/phase14c-ode-pilot \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUNDIALS_ROOT=/home/142401034/.local/numerix-deps/prefix/phase14/sundials
cmake --build build/phase14c-ode-pilot --parallel
./build/phase14c-ode-pilot/ode_sundials_pilot
```

The program first checks the loaded SUNDIALS ERK Butcher coefficients against
the respective NUMERIX schemes: forward Euler, Heun's explicit trapezoid, and
classical RK4. It then runs both libraries on the same fixed two-state system,
fixed step, initial state, interval and output grid. It prints maximum absolute
trajectory errors, callback counts, and the maximum cross-library trajectory
difference. This checks adapter dispatch and the fixed-step SUNDIALS setup; it
does not satisfy the Phase 14C workload or timing, memory, or statistical gates.

## Current public-method audit

The full API mapping and memory caveats are recorded in
[`optimization_ode_audit.md`](../optimization_ode_audit.md). The inventory has
five numerical methods: two optimizers and three fixed-step ODE integrators.
`numerix_ode_solution_destroy` and vector/interpolant cleanup routines are
lifecycle operations, not additional numerical algorithms.
