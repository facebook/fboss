# Test Categorization Guide (Meta External)

## When to Use

After completing debug iterations for a test, to assign the final category and update the session tracking table.

## Categorization Taxonomy

### PASS_NO_CHANGE
**Test passes without any code or config change.**
- The test works as-is on the target platform
- No action needed — record and move on

### PASS_FEATURE_FLAG
**Test passes after changing an HwAsic feature flag.**
- The platform's `HwAsic` didn't have the right feature flags set
- Fix: Update the feature flag in the platform's HwAsic configuration
- Example: Enabling `SAI_ACL_ENTRY_ATTR_FIELD_*` for a new ASIC

### PASS_FBOSS_FIX
**Test passes after fixing FBOSS agent code.**
- A bug or missing handling in FBOSS code caused the failure
- Fix is in FBOSS-owned code (agent, SAI wrapper, test infrastructure)
- Document the diff/change that fixed it

### PASS_VENDOR_FIX
**Test passes after fixing vendor SDK code.**
- A bug or missing feature in the vendor SAI/SDK caused the failure
- Fix requires vendor SDK code change
- Document the vendor fix or workaround applied

### FAIL_FBOSS
**Identified as FBOSS issue, needs more work.**
- Root cause is in FBOSS code but fix is non-trivial
- Used when 5 iterations aren't enough to resolve
- Document findings so far for follow-up

### FAIL_VENDOR
**Identified as vendor SDK issue, needs more work.**
- Root cause is in vendor SAI/SDK code
- May need to file a vendor bug or wait for a fix
- Document evidence (SAI Replayer logs, diag shell output)

### FAIL_TIMEOUT
**Test did not complete within the allowed time.**
- The test binary hung or took too long (cold boot or warm boot)
- May indicate a deadlock, infinite loop, or SDK initialization hang
- Check for stuck processes on the switch, review logs for the last action before hang

### TODO_ASIC_CONFIG
**May need ASIC chip configuration change.**
- Suspected that lane map, MMU settings, or other ASIC config needs adjustment
- Requires consultation with platform/hardware team
- Document the suspected config area

## Session Tracking Template

Save to `/tmp/agent_hw_test_results.md` and update throughout the session:

```markdown
# AgentHwTest Debug Session
**Date**: YYYY-MM-DD
**Switch**: <switch-name>
**Platform**: <platform>
**Binary**: <mono|multi>

## Results

| # | Test Name | Switch ID | Cold Boot | Warm Boot | Category | Iterations | Notes |
|---|-----------|-----------|-----------|-----------|----------|------------|-------|
| 1 | | | | | | | |

## Summary
- Total tests: 0
- PASS_NO_CHANGE: 0
- PASS_FEATURE_FLAG: 0
- PASS_FBOSS_FIX: 0
- PASS_VENDOR_FIX: 0
- FAIL_FBOSS: 0
- FAIL_VENDOR: 0
- FAIL_TIMEOUT: 0
- TODO_ASIC_CONFIG: 0
- Pass rate: 0/0
```

## Decision Tree

Use this to guide categorization:

1. Did the test pass on first run? → `PASS_NO_CHANGE`
2. Did it pass after changing only a feature flag? → `PASS_FEATURE_FLAG`
3. Did it pass after an FBOSS code change? → `PASS_FBOSS_FIX`
4. Did it pass after a vendor SDK change? → `PASS_VENDOR_FIX`
5. Is the root cause identified but not yet fixed?
   - In FBOSS code → `FAIL_FBOSS`
   - In vendor SDK → `FAIL_VENDOR`
6. Did the test hang or timeout? → `FAIL_TIMEOUT`
7. Might it need ASIC config changes? → `TODO_ASIC_CONFIG`

## Next Steps

- Update the session tracking table with the category
- If all tests are categorized, generate a summary report
- File tasks/bugs for `FAIL_*` and `TODO_*` categories
