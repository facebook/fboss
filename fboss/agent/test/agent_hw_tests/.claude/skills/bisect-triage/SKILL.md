---
name: bisect-triage
description: Triage a failing FBOSS test by bisecting commits and investigating code changes. Use when a test is broken and you need to find the commit that caused the failure.
argument-hint: "[test-regex] [bad-rev] [good-rev]"
disable-model-invocation: true
user-invocable: true
---

# FBOSS Test Failure Triage

Triage a test failure by running two parallel threads:
1. **Bisect** — Use `bisect2` to find the exact breaking commit
2. **Code investigation** — Search recent commits and test code to identify likely culprits

## Output Format

**IMPORTANT**: Always present the bisect2 command first at the top of your output so the user can kick it off immediately while you continue investigating. The bisect takes a long time to run, so getting it started early saves significant time.

Structure your output as:

1. **Bisect command** (first thing shown, ready to copy-paste)
2. Code investigation findings (while bisect runs)
3. Log analysis (if log file provided)
4. Full report with culprit analysis, failure modes, and next steps

## Information Gathering

Before starting, collect the following from the user. Ask for any missing pieces:

- **Test regex**: Pattern matching the failing test name(s) (for `--regex`)
- **Bad revision**: Commit hash where the test is known to fail
- **Good revision**: Commit hash where the test last passed
- **Netcastle team**: e.g., `bcm_agent_test`, `sai_agent_test` (default: `bcm_agent_test`)
- **Test config**: e.g., `tomahawk3_alpm/6.5.26_6.5.26`
- **Buck mode**: e.g., `opt-asan` (default: `opt-asan`)
- **Netcastle node name**: The conveyor node name if available (for context)
- **Log file**: If the user has a netcastle log file, read it for failure details (see Log Analysis below)

If the user provides arguments via the slash command, parse them as: `/bisect-triage <test-regex> <bad-rev> <good-rev>`

## Thread 1: Bisect

**Present this command to the user FIRST before doing any investigation.**

### bisect2 Tool

The bisect2 tool is at:
```
buck2 run fbcode//fboss/util/facebook/devdbgtest:fboss-dev-helper bisect2
```

**CRITICAL: Must be run from `fbsource/` root (NOT `fbcode/`).** The `--bisect_dirs` flag uses repo-root-relative paths (e.g., `fbcode/fboss`), and the internal `hg log` command silently returns zero commits if the path doesn't exist relative to CWD.

**Required positional args:**
- `GOOD_COMMIT` — commit hash where the test passes
- `BAD_COMMIT` — commit hash where the test fails

**Key options:**
| Option | Description |
|--------|-------------|
| `--netcastle_cmd "<cmd>"` | Netcastle command string to use as the test oracle |
| `--job N` | N-ary parallel bisect (N>1 requires `--use_sandcastle`) |
| `--use_sandcastle` | Run builds/tests remotely via Sandcastle |
| `--retry N` | Retry UNKNOWN results (Sandcastle only) |
| `--bisect_dirs "dir1 dir2"` | Scope commit history to specific directories (must start with `fbcode/`) |

**Validation rules:**
- Exactly one of `--fbpkg`, `--buck_target`, or `--netcastle_cmd` must be specified
- `--job > 1` requires `--use_sandcastle`

### Assembling the bisect2 command

Construct the command like this:
```bash
cd ~/fbsource  # MUST be at repo root
buck2 run fbcode//fboss/util/facebook/devdbgtest:fboss-dev-helper bisect2 \
  <GOOD_COMMIT> <BAD_COMMIT> \
  --use_sandcastle \
  --netcastle_cmd "netcastle --team <TEAM> --jobs 12 --test-config <TEST_CONFIG> --buck-mode <BUCK_MODE> --regex '<TEST_REGEX>'" \
  --bisect_dirs "fbcode/fboss"
```

Suggest `--job 5` for faster parallel bisect when using Sandcastle.

### Quick Verification (skip full bisect)

If code investigation has identified a likely culprit, determine which type of breakage it is:

**Type 1: True regression** — tests that previously passed now fail. Use bisect2 with just the suspect and parent:

```bash
cd ~/fbsource  # MUST be at repo root

# Get the parent of the suspect commit (use full hash, not short)
sl log --rev 'parents(<SUSPECT_COMMIT>)' -T '{node}\n'

# Run bisect2 with the narrow 2-commit range
buck2 run fbcode//fboss/util/facebook/devdbgtest:fboss-dev-helper bisect2 \
  <PARENT_COMMIT> <SUSPECT_COMMIT> \
  --use_sandcastle \
  --netcastle_cmd "netcastle --team <TEAM> --jobs 12 --test-config <TEST_CONFIG> --buck-mode <BUCK_MODE> --regex '<TEST_REGEX>' --skip-filtering-by-test-state --run-disabled"
```

**Type 2: Newly enabled tests that are broken on arrival** — a commit enables tests on a new platform but they fail. Bisecting against the parent won't work because the tests didn't exist before. Instead, just reproduce the failure at the current revision:

```bash
netcastle --team <TEAM> --jobs 12 \
  --test-config <TEST_CONFIG> --buck-mode <BUCK_MODE> \
  --basset-query <BASSET_QUERY> \
  --regex '<TEST_REGEX>' \
  --skip-filtering-by-test-state --run-disabled
```

The proof for Type 2 is:
1. The test enablement code was added in the suspect commit (read the diff to confirm)
2. The failure reproduces at the current revision
3. Optionally, the commit's own test plan already shows failures

### Netcastle arguments reference

These optional netcastle arguments may be relevant depending on the use case:

| Argument | Purpose |
|----------|---------|
| `--update-agent-fbpkg-info <pkg>` | Logs the agent fbpkg version to Scuba (metadata only, no behavior change) |
| `--run-disabled` | Include tests disabled by Test Console |
| `--basset-query <query>` | Target specific lab devices, e.g., `fboss.kernel.legacy/asic=tomahawk3` |
| `--skip-filtering-by-test-state` | Skip Test Console state filtering, run all tests |
| `--regex <pattern>` | Filter tests by regex (maps to `--gtest_filter` for GTest). Multiple `--regex` flags act as AND |
| `--purpose <purpose>` | Test purpose context, e.g., `continuous` (for conveyor runs) |

Conveyor pipeline runs typically use all of these flags together:
```bash
netcastle --team bcm_agent_test --purpose continuous --jobs 12 \
  --test-config <CONFIG> --buck-mode opt-asan \
  --update-agent-fbpkg-info <FBPKG_ID> \
  --run-disabled --basset-query <BASSET_QUERY> \
  --skip-filtering-by-test-state
```

## Thread 2: Code Investigation

While bisect runs, investigate in parallel:

1. **List commits in the range** touching fboss:
   ```bash
   sl log --rev '<good>::<bad> & file("fbcode/fboss/**")' -T '{node|short} {date|isodate} {desc|firstline}\n' -l 100
   ```

2. **Search for keyword-relevant commits** using keywords from the failing test names (e.g., ecmp, spillover, warmboot, resource):
   ```bash
   sl log --rev '<good>::<bad> & file("fbcode/fboss/**")' -T '{node|short} {date|isodate} {desc|firstline}\n' --keyword <keyword>
   ```

3. **Read the failing test source code** to understand what the test verifies. Use `search_files` MCP tool to find test class definitions.

4. **Check if the test code itself was modified** in the commit range.

5. **Look at the diff of suspicious commits**:
   ```bash
   sl diff --rev 'parents(<commit>) & ancestors(<commit>)' --rev <commit>
   ```

6. **Check the Phabricator diff** for the suspicious commit — look at the test plan to see if the author noticed failures.

## Known Bad / Unsupported Test Config

Check whether the failing tests are already marked as known bad or unsupported in configerator. The config files are in the user's configerator checkout (typically `~/configerator/`).

### Config file locations

| Team | Config File |
|------|------------|
| `bcm_agent_test` | `~/configerator/source/neteng/netcastle/test_config/bcm_agent_test.cinc` |
| `sai_agent_test` | `~/configerator/source/neteng/netcastle/test_config/sai_agent_test.cinc` |

### Structure

Each config file defines:
- **`KnownBadTestPattern`** — tests known to fail (skipped in results, regex matched against test name)
- **`UnsupportedTestPattern`** — tests not supported on a platform (never run)

Tests are organized per platform config. For example in `bcm_agent_test.cinc`:
```python
bcm_common_known_bad_tests = [...]          # Shared across all BCM platforms
bcm_th3_known_bad_tests = [...]             # TH3-specific known bads
bcm_th4_known_bad_tests = [...]             # TH4-specific known bads

bcm_agent_test_known_bad_tests_map = {
    "tomahawk3_alpm/6.5.26_6.5.26": bcm_common_known_bad_tests + bcm_th3_known_bad_tests,
    "tomahawk4_alpm/6.5.26_6.5.26": bcm_common_known_bad_tests + bcm_th4_known_bad_tests,
}
```

For `sai_agent_test.cinc`, the structure is similar but with per-SDK and per-ASIC-family lists.

### How to check

Search the config for the failing test name:
```bash
grep -i '<TEST_CLASS_NAME>' ~/configerator/source/neteng/netcastle/test_config/<TEAM>.cinc
```

If the test is **not listed**, it's expected to pass. If it's failing and not in known bad, it's a real regression that needs to be fixed or added to known bad.

### Adding to known bad

To add a test to known bad, edit the appropriate list in the `.cinc` file. Use a regex pattern that matches the test name:
```python
bcm_th3_known_bad_tests = [
    KnownBadTestPattern(test_name_regex="{}$".format(test_group))
    for test_group in [
        # ... existing entries ...
        "AgentEcmpSpilloverTest.*",  # TODO(author): fix TH3 spillover tests, D92924806
    ]
]
```

Then submit the configerator change via the normal diff workflow. Always include a TODO with the author and context so it gets fixed rather than forgotten.

## Log Analysis

When the user provides a netcastle log file, analyze it for failure clues. The log is typically large (1MB+), so use Grep rather than reading the entire file.

### Key patterns to search for in logs

**1. Find all FAILED and PASSED tests:**
```
grep FAILED <logfile>
grep PASSED <logfile>
```

**2. Identify the failure mode** — check which type of failure:
- `exit-code exited 1` → Test assertion failure (GTest EXPECT/ASSERT)
- `signal killed ABRT` → Process crash, usually a `LOG(FATAL)` or ASan error
- `timeout` → Test hung or exceeded time limit

**3. Find assertion details:**
```
grep -A 5 "Check failed\|EXPECT_EQ\|ASSERT_EQ\|Expected equality" <logfile>
grep -B 5 "ABORTING" <logfile>
```

**4. Check for ASan (AddressSanitizer) errors:**
```
grep -A 15 "ERROR: AddressSanitizer" <logfile>
grep "heap-buffer-overflow\|use-after-free\|stack-buffer-overflow" <logfile>
```

**5. Find crash stack traces** — look for `LOG(FATAL)` abort paths:
```
grep -A 10 "SwSwitchInitializer::initThread\|folly::LogStreamVoidify" <logfile>
```

**6. Extract the runner command** (shows exact flags used):
```
grep "Runner CMD:" <logfile>
```

**7. Get the fbcode hash** (confirms the revision under test):
```
grep "Fbcode hash:" <logfile>
```

**8. Check for core dumps** (useful for further debugging):
```
grep -A 2 "Core dumps:" <logfile>
```

**9. Find everpaste links** for full stdout/stderr (log file only shows last N lines):
```
grep "everpaste.*handle=" <logfile>
```

### Common failure patterns

| Pattern | Typical Cause |
|---------|---------------|
| warm_boot fails but cold_boot passes for same test | Warm boot initialization or state replay issue |
| All tests in a test class fail | Test class setup/config issue (e.g., wrong switching mode, wrong resource limits) |
| `SwSwitchInitializer::initThread` in crash stack | Fatal error during switch warm boot initialization |
| ASan ABORTING with no ASan error | Usually a `LOG(FATAL)` / `CHECK()` failure, not a memory error — the ASan legend in output is from the binary being built with ASan, not from an ASan detection |
| Tests that were previously not running now fail | Newly enabled tests that weren't validated on this platform |

## Reporting

Present findings as:

1. **Most likely culprit commit** with diff number, title, author, and evidence
2. **What the commit changed** and why it likely causes the failure
3. **Failure mode** — assertion failure vs crash vs timeout, with key error messages
4. **Cold boot vs warm boot pattern** — which boot modes are affected
5. **The bisect2 command** ready to run for confirmation
6. **Suggested next steps** (talk to author, revert, fix forward, etc.)
