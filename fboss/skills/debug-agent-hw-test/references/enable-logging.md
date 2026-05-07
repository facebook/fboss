# Enable Logging (Meta External)

## When to Use

When a test **fails** and the initial log analysis ([analyze-logs.md](analyze-logs.md)) does not yield a root cause. These options add verbose logging but come with caveats — use sparingly.

**Each option requires re-running the test.** Options can be combined.

## Logging Options

### (A) SDK Verbose Logging — `--enable_sai_log DEBUG`

Enables verbose logging from the vendor SDK. Produces detailed SAI API traces.

Add to the test command:
```bash
--enable_sai_log DEBUG
```

**Inspect**: test binary output **and** `/var/log/messages` on the switch for suspicious error logs.

**When to use**: when you suspect the issue is in vendor SDK behavior and need to see internal SDK decisions.

### (B) FBOSS Verbose Logging — `--logging='fboss=DBG5'`

Enables verbose FBOSS agent logging at debug level 5.

Add to the test command:
```bash
--logging='fboss=DBG5'
```

**Inspect**: test binary output for detailed FBOSS agent traces.

**When to use**: when you suspect the issue is in FBOSS agent logic (state machine, config application, handler code).

### (C) SAI Get Attribute Logging — `--enable_get_attr_log`

Enables logging of SAI get attribute calls in the SAI replayer log.

Add to the test command:
```bash
--enable_get_attr_log
```

**Inspect**: SAI replayer logs (`sai_replayer-cb.log` / `sai_replayer-wb.log`) — they will contain additional get attribute entries.

**When to use**: when you need to see what values the agent is reading back from the SDK, not just what it's writing.

### (D) Packet TX Logging — `--enable-packet_log`

Enables logging of SAI packet transmit calls.

Add to the test command:
```bash
--enable-packet_log
```

**Inspect**: test binary output for packet TX entries.

**When to use sparingly**: to rule out whether packets are being sent at all. Tests that send large numbers of packets can overwhelm the logs with this option enabled.

## Example: Combining Options

Re-run a failing cold boot test with SDK and FBOSS verbose logging:

```bash
/root/<user>/<versioned-binary-name> \
  --config <config-path> \
  --gtest_filter=<TestSuite>.<TestName> \
  --enable-replayer --sai_log /root/<user>/sai_replayer-cb.log \
  --enable_sai_log DEBUG \
  --logging='fboss=DBG5' \
  --setup-for-warmboot
```

## Decision Guide

| Symptom | Try Option |
|---------|------------|
| SAI call returns unexpected error | (A) SDK logs |
| Agent logic seems wrong | (B) FBOSS logs |
| Need to see values read from SDK | (C) Get attribute logs |
| Packets not arriving / test expects packet | (D) Packet TX logs |
| No clue where to start | (A) + (B) together |

## Next Steps

- Re-run the failing test with chosen logging options
- Analyze the enhanced logs with [analyze-logs.md](analyze-logs.md)
- If hypothesis formed: see [hypothesis-driven-debug.md](hypothesis-driven-debug.md)
