# Root run excluded from final analysis

This run contains complete 31-batch data for the planned 17 library-method combinations, but its NUMERIX discovery settings exceeded the API's documented maximum subdivision count and were rejected as invalid input. The run is retained intact; it is not used in final summaries. The corrected run lowers `maximum_subdivisions` to the verified API maximum and initializes discovery metrics for every library. Related provisional summaries are retained under `benchmarks/results/phase14b/analysis/root-incomplete-discovery-options-20261112/`.
