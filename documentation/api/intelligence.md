# Intelligence API

See [the Phase 4 architecture guide](../architecture/intelligence.md) for recommendation rules, limits, fallback behavior, and ownership.

Typical root recommendation and execution:

```c
numerix_recommendation_request request = {0};
numerix_recommendation recommendation;
numerix_execution_output output;
numerix_error error;

request.category = NUMERIX_PROBLEM_ROOT_FINDING;
request.representation = NUMERIX_REPRESENTATION_CALLBACK;
request.function = my_function;
request.problem.root.lower = 0.0;
request.problem.root.upper = 2.0;
request.problem.root.has_bracket = 1;

numerix_recommend_algorithm(&request, &recommendation, &error);
numerix_execute_recommended(&request, NULL, &output, &error);
/* Use output.scalar_result and output.recommendation. */
numerix_execution_output_destroy(&output);
```

Use `numerix_root_selection_options` to force an algorithm or enable recoverable root fallback. `numerix_root_solve_portfolio` compares independent root attempts and returns their full history. Portfolio attempts are sequential and find at most one root per attempt; they do not discover all roots in an interval.

For expression input, set `representation` to `NUMERIX_REPRESENTATION_EXPRESSION` and provide a borrowed compiled expression. For piecewise input, provide the borrowed piecewise object and select `NUMERIX_REPRESENTATION_PIECEWISE`. The caller retains and destroys these objects. Numerical execution output owns any vector or ODE storage until `numerix_execution_output_destroy` is called.
