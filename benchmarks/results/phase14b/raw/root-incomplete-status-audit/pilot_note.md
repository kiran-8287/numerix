# Incomplete root method run

This directory retains a second stopped pilot. It contains partial 31-batch data from the first method suite. Review during the run found that the GSL iteration-cap path could report an unsuccessful capped solve as converged. The adapter was corrected to mark cap exhaustion as a failure, and the entire pilot is excluded from final Phase 14B root summaries. It remains intact for auditability. The corrected study is stored separately under `root-method-study-fixed-20261112`.
