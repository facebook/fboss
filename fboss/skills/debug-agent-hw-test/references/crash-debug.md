# Crash Debugging (Meta External)

## When to Use

When a test crashes (segfault, SIGABRT, uncaught exception) and you need to determine the crash location and root cause.

## Non-Stripped Binaries

For useful stack traces, you need a non-stripped (debug) binary.

### Deploying Debug Binaries

Debug binaries are larger. Ensure sufficient disk space on the switch:

    RUN ON <switch>: df -h /tmp

See the resolved device-access reference selected via the Reference
Routing table in `SKILL.md` for the actual command in your
environment.

## Getting Stack Traces

### From Core Dumps

Check for core dumps on the switch and analyze with GDB:

    RUN ON <switch>: ls /tmp/core.* 2>/dev/null

### Using GDB

Run GDB in batch mode on the switch to get a backtrace:

    RUN ON <switch>: gdb -batch -ex 'bt full' /tmp/<binary> /tmp/core.<pid>

For longer GDB sessions, use a timeout of at least 120 seconds.

### Common GDB Commands

```
bt              # Full backtrace
bt full         # Backtrace with local variables
frame N         # Select frame N
info locals     # Show local variables in current frame
print <expr>    # Print expression value
```

## Common Crash Patterns

| Signal | Common Cause | Investigation |
|--------|-------------|---------------|
| SIGSEGV | Null pointer dereference | Check pointer validity in backtrace |
| SIGABRT | Assertion failure or CHECK | Read the CHECK/DCHECK message |
| SIGBUS | Misaligned memory access | Check hardware-related code |

## Expected Output

- Stack trace showing the exact crash location
- Local variable values at the crash point
- Identification of the root cause (null ptr, assertion, etc.)

## Next Steps

- If root cause is in FBOSS code: fix and rebuild via [build-and-load.md](build-and-load.md)
- If root cause is in vendor SDK: categorize as `FAIL_VENDOR` or `PASS_VENDOR_FIX`
- If unclear: add targeted XLOGs via [hypothesis-driven-debug.md](hypothesis-driven-debug.md)
