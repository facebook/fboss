# Vendor Diagnostic Shell (Meta External)

## When to Use

When you need to inspect hardware state directly — counters, route/neighbor entries, port status — to verify what the ASIC actually programmed vs what FBOSS intended.

## Accessing the Diagnostic Shell

<!-- TODO: User to provide how to access the vendor diag shell on different platforms -->

## Counter Commands

Check hardware counters to understand packet flow and drops:

<!-- TODO: User to provide specific counter commands -->

### Port Counters

<!-- TODO: Commands to check per-port TX/RX counters, error counters -->

### Drop Counters

<!-- TODO: Commands to check drop reasons and counts -->

## Route/Neighbor Entry Verification

Verify that routes and neighbor entries are correctly programmed in hardware:

<!-- TODO: User to provide commands to dump route and neighbor tables -->

### Route Table

<!-- TODO: Commands to check L3 route entries -->

### Neighbor Table

<!-- TODO: Commands to check L2/L3 neighbor entries -->

## Port Status

Check physical port state, link status, and configuration:

<!-- TODO: User to provide port status commands -->

## ACL/QoS Verification

<!-- TODO: User to provide commands to check ACL and QoS programming -->

## Common Diagnostic Patterns

<!-- TODO: User to provide common diagnostic patterns -->

| Symptom | Diag Command | What to Look For |
|---------|-------------|-----------------|
| Packets not forwarded | Check route table | Missing or wrong route entry |
| Packets dropped | Check drop counters | Specific drop reason |
| Wrong port behavior | Check port config | Mismatched speed/FEC settings |

## Expected Output

- Hardware state that can be compared against FBOSS agent's intended state
- Specific counter values or table entries that explain the test failure

## Next Steps

- If hardware state doesn't match FBOSS intent: investigate agent programming logic
- If hardware state is correct but test still fails: re-examine test expectations
- If vendor SDK bug suspected: categorize as `FAIL_VENDOR` with diagnostic evidence
