# Hypothesis-Driven Debugging (Meta External)

## When to Use

When standard log analysis hasn't revealed the root cause and you need to form and test specific hypotheses by adding targeted logging, modifying behavior, or isolating variables.

## The Process

1. **Form a hypothesis** — Based on available evidence, state what you think is happening
2. **Design a test** — Add XLOGs, modify a condition, or isolate a variable to confirm or deny
3. **Implement** — Make the minimal change to test the hypothesis
4. **Run** — Execute the test with the change
5. **Evaluate** — Did the evidence confirm or deny the hypothesis?
6. **Iterate** — Refine hypothesis based on new evidence

## Adding XLOG Statements

Insert targeted XLOG statements to trace execution flow and variable values:

```cpp
// Trace entry to a function
XLOG(INFO) << "[DEBUG] Entering functionName, param=" << param;

// Trace a conditional branch
XLOG(INFO) << "[DEBUG] condition=" << condition << " taking branch X";

// Trace a value at a specific point
XLOG(INFO) << "[DEBUG] value at checkpoint: " << value;
```

### Guidelines

- Use `[DEBUG]` prefix so debug logs are easy to find and remove later
- Log at `INFO` level so logs appear without changing log level configuration
- Log variable values, not just "reached here" — values are more diagnostic
- Keep changes minimal — one hypothesis at a time

## Modifying Behavior

Sometimes the best way to test a hypothesis is to change behavior:

- **Skip a code path**: Comment out a suspected problematic section
- **Hardcode a value**: Replace a computed value with a known-good constant
- **Add a sleep**: Insert a delay to test race condition hypotheses
- **Change order**: Swap operation order to test dependency hypotheses

## Build-Run Cycle for Hypothesis Testing

1. Make the minimal code change (XLOG or behavior modification)
2. Build with [build-and-load.md](build-and-load.md)
3. Run with [run-tests.md](run-tests.md)
4. Analyze output — did the hypothesis hold?
5. Revert debug changes before making the real fix

## Tracking Hypotheses

Document each hypothesis and result in the session tracking table:

```
Hypothesis 1: Route programming fails because nexthop isn't resolved
  Evidence: XLOG shows nexthop resolution returns null
  Result: CONFIRMED — nexthop resolution not called before route add
  Action: Add nexthop resolution call before route programming
```

## Expected Output

- Confirmation or denial of specific hypotheses
- Narrowed root cause after 1-3 hypothesis iterations
- Clear direction for the fix

## Next Steps

- If hypothesis confirmed and fix is clear: implement fix, rebuild via [build-and-load.md](build-and-load.md)
- If hypothesis denied: form new hypothesis based on new evidence
- If 5 iterations exhausted without resolution: categorize as `FAIL_FBOSS` or `FAIL_VENDOR` and move to next test
