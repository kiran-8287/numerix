# Incomplete root method run excluded from final analysis

This directory retains a partial 31-batch run that was stopped after a review found callback evaluation counters were still enabled inside timed calls. That instrumentation can add method-dependent callback overhead. The GSL iteration-cap status handling was also corrected before the final run. The accuracy probes and partial timings remain available for audit but are not pooled into final results. The corrected run disables counters in warmups/timing and marks capped solves as non-converged.
