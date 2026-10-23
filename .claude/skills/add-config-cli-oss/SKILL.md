---
name: add-config-cli-oss
description: Implement a new `fboss2-dev config <area> <attr> <value>` CLI command family mapping to `SwitchConfig` attributes. Figures out the correct action level (HITLESS / AGENT_WARMBOOT / AGENT_COLDBOOT), wires the handler through the full registration + build graph, writes unit + integration tests, runs them on a test switch, and prepares the change for review.
when_to_use: |
  User asks to add a new CLI command (or family of related commands) under
  `fboss2-dev config ...` that sets one or more fields on the agent's
  `SwitchConfig` object. Typical request: "implement a CLI for
  configuring <something>".
argument-hint: [subcommand-spec] [branch] [test-switch] [sample-config]
---

# Adding a new `fboss2-dev config <area>` CLI command family

## 0. Inputs

- A spec of the desired subcommand(s), ideally spelling out the
  `fboss2-dev config <area> <attr> <value>` shape. Related subcommands
  (e.g. a family of timers on the same Thrift object) are best implemented
  together in one change.
- **Optional**: A git branch based on `main`. Will be created automatically
  if it doesn't exist yet.
- **Optional**: the name of a test switch that is already provisioned with a
  recent FBOSS image built from the branch. If provided, skip straight to
  the integration-testing step in §6.
- **Optional but strongly preferred**: a path to a real production FBOSS
  `agent.conf` from a deployed device. This anchors the tests to the config
  shape the CLI must actually be able to drive in production.
  - Do **not** read the file end-to-end — production configs are hundreds
    of thousands of lines. Wait until §1, where you've identified the
    Thrift fields of interest, and then use the `Grep` tool with the
    field names (`cpuQueues`, `cpuTrafficPolicy`, `loadBalancers`, etc.)
    to locate the relevant sub-trees. Read a small window around each
    hit (≤200 lines) to capture the nested shape and any unusual
    per-field values.
  - Use what you find to shape the seed JSON in the §4 unit-test fixture
    and the target fields in the §4 integration test — so that the unit
    test proves the CLI mutates a config matching production, and the
    integration test operates on a device state that resembles production.
  - If the user provides only a CLI spec and no sample config, ask whether
    one is available before falling back to an invented fixture —
    production shapes surface edge cases (union fields, ordered lists,
    optional wrappers) that synthetic fixtures miss.

## 1. Map CLI names → Thrift fields

Map each CLI attribute onto the `SwitchConfig` Thrift field it should
mutate, by code exploration:

1. Read [fboss/agent/switch_config.thrift](../../../fboss/agent/switch_config.thrift)
   and grep for keywords from the CLI spec (the attribute name, the area
   name, related protocol terms). Field names usually track the CLI
   vocabulary closely (e.g. `timeout` → `arpTimeoutSeconds`,
   `age-interval` → `arpAgerInterval`).
2. Cross-check candidates in `fboss/agent/ApplyThriftConfig.cpp` — the
   place a field is *consumed* confirms both that you found the right
   field and what it actually controls.
3. When a field lives inside a nested struct/list/union (e.g.
   `loadBalancers[id=...].fieldSelection.ipv4Fields`), write down the
   full JSON path from the `sw` root — the handler will need to navigate
   or create that path.

Example mapping (ARP family):

| CLI attr         | SwitchConfig field       |
|------------------|--------------------------|
| `timeout`        | `arpTimeoutSeconds` (i32) |
| `age-interval`   | `arpAgerInterval` (i32)   |
| `max-probes`     | `maxNeighborProbes` (i32) |
| `stale-interval` | `staleEntryInterval` (i32)|

Note the field *types* (i32 / i64 / enum / bool) — the arg-validation
code uses them.

**Now** is the time to grep the sample production config (if one was
provided in §0) for the field names you just mapped. Run
`Grep pattern="<field1>\|<field2>" path=<sample-config>` and read a
small window (≤200 lines) around each hit to capture the nested shape,
ordering, and any unusual values. The JSON you see here is what the
unit-test seed config in §4 should mirror, and it tells you which
specific fields the integration test in §4 is most likely to find on a
real device.

## 2. Determine `cli::ConfigActionLevel`

For each field, answer: **can the agent apply this change at runtime, or does
it need a warmboot/coldboot?**

Check, in order:

1. `fboss/agent/ApplyThriftConfig.cpp` — find where the field is read. If
   it's just a `newSwitchSettings->set<Field>(...)` style assignment, it's
   almost certainly HITLESS.
   If the field is missing from `ApplyThriftConfig.cpp` entirely, it is
   **dead code in the agent** — raise this with the user before wiring the
   CLI (see §2a below).
2. `fboss/agent/hw/sai/switch/SaiSwitch.cpp` — grep for `<field>ChangeProhibited()`
   or `<field>` appearing next to a `FbossError` throw inside a
   `SwitchSettingsDelta` handler. If one exists, the field can't change
   hitlessly — use `AGENT_COLDBOOT` (or `AGENT_WARMBOOT` if the comment
   allows).
3. `fboss/agent/state/SwitchSettings.h` — the setter should be a simple
   `set<tag>(value)`. Anything more interesting means look harder.

The three action levels live in
[fboss/cli/fboss2/cli_metadata.thrift](../../../fboss/cli/fboss2/cli_metadata.thrift):
`HITLESS`, `AGENT_WARMBOOT`, `AGENT_COLDBOOT`.

Your own code-path investigation (ApplyThriftConfig, SaiSwitch,
SwitchSettings) is the source of truth — treat any level someone guessed
up front as a hint, not fact, and make sure the handler calls
`saveConfig(...)` with the level the code paths actually support.

### 2a. Dead-code attributes

Some SwitchConfig fields are declared but never applied by the agent (e.g.
a field present in `switch_config.thrift` but absent from
`switch_state.thrift` and never read by `ApplyThriftConfig.cpp`). Wiring a
CLI for these produces a no-op. Ask the user whether to:

1. Implement the CLI anyway + add a warning comment
2. Expand the change to wire the agent side too
3. Drop that subcommand from scope and defer

## 3. Pick a structure

For a *family* of related scalar tunables on one Thrift object (like the 5
ARP timers), use a **single handler class with a `kValidAttrs` set** rather
than one class per subcommand. The reference implementation is
[fboss/cli/fboss2/commands/config/arp/CmdConfigArp.{h,cpp}](../../../fboss/cli/fboss2/commands/config/arp/CmdConfigArp.h).

For a single one-off command with a complex arg shape, follow
[fboss/cli/fboss2/commands/config/l2/learning_mode/CmdConfigL2LearningMode.h](../../../fboss/cli/fboss2/commands/config/l2/learning_mode/CmdConfigL2LearningMode.h).

## 4. Files to touch (family pattern)

### No string literals in handler code

Code review style rejects literal string constants sprinkled inside
handler logic — lift them to named `constexpr std::string_view` in an
anonymous namespace at the top of the `.cpp`. See
[fboss/cli/fboss2/session/FbossServiceUtil.cpp](../../../fboss/cli/fboss2/session/FbossServiceUtil.cpp)
for the canonical pattern (e.g. `kWedgeAgent`, `kSwAgent`,
`kHwAgentPrefix`). Apply the same style to every CLI attribute name
the dispatch code compares against, and build `k<Area>ValidAttrs`
from those constants rather than raw strings.

### New files

1. `fboss/cli/fboss2/commands/config/<area>/CmdConfig<Area>.h`
   - Declare the `class <Area>ConfigArgs`, the `CmdConfig<Area>Traits`
     struct, and the `CmdConfig<Area>` handler. Keep implementation detail
     out of the header; the exception is the attribute-name constants when
     they must be shared with another command tree (e.g. `config arp` and
     `delete arp` share a `namespace arp_attrs` of
     `constexpr std::string_view` names plus a `kValidAttrs` array in the
     header — see `CmdConfigArp.h`). If nothing else needs them, keep them
     in the `.cpp`.
   - `class <Area>ConfigArgs : public utils::BaseObjectArgType<std::string>`
     with a `std::vector<std::string>` ctor that validates exactly 2 args,
     first ∈ valid set, second parses to a non-negative `int32_t` via
     `folly::to<int32_t>` (catch `folly::ConversionError`).
   - `struct CmdConfig<Area>Traits : public WriteCommandTraits` with
     `ObjectArgType = <Area>ConfigArgs`, `RetType = std::string`, and a
     `static void addCliArg(CLI::App& cmd, std::vector<std::string>& args)`
     method that registers the positional via
     `cmd.add_option("<area>_attr_value", args, "<help text>")`. This is
     where the CLI11 registration lives now — there is no central enum or
     switch to touch (config commands migrated off the old
     `ObjectArgTypeId` mechanism in commit `5b534a2aad`). Chain
     `->required()->expected(N)` when the arg count is fixed (see the
     CLI11 pitfall in §8).
   - `class CmdConfig<Area> : public CmdHandler<CmdConfig<Area>, CmdConfig<Area>Traits>`
     declaring `queryClient(HostInfo, ObjectArgType)` + `printOutput(RetType)`.

2. `fboss/cli/fboss2/commands/config/<area>/CmdConfig<Area>.cpp`
   - `#include "fboss/cli/fboss2/CmdHandler.cpp"` — important, this is how the
     template gets instantiated
   - The attribute-name constants and valid-attrs set (unless they had to
     live in the header for sharing, per above): one
     `constexpr std::string_view k<Area>Attr<Name>` per CLI attribute name
     in an anonymous namespace, followed by a valid-attrs container built
     from those constants.
   - `<Area>ConfigArgs` constructor body (use the constants for error
     messages + membership checks).
   - `queryClient()` body: get `ConfigSession::getInstance()`, dispatch on
     `args.getAttribute()` against the named constants (not string
     literals), mutate `config.sw()->*` fields, call
     `session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::<LEVEL>)`,
     return success string.
   - `printOutput()` → `std::cout << msg << std::endl;`
   - Trailing `template void CmdHandler<..., ...>::run();` explicit
     instantiation

3. `fboss/cli/fboss2/test/config/CmdConfig<Area>Test.cpp` — inherit
   `CmdConfigTestBase` with a seed-config JSON, test arg validation
   (valid / bad arity / unknown attr / non-integer / negative) and `queryClient()`
   for each attr. See
   [CmdConfigArpTest.cpp](../../../fboss/cli/fboss2/test/config/CmdConfigArpTest.cpp).
   - **Shape the seed JSON after the production config** (grepped from
     the §0 sample in §1). Copy enough of the sub-tree that each attr's
     `queryClient()` has something realistic to mutate: present
     optionals, populated ordered-lists, existing enum values. A
     reviewer should be able to eyeball the seed and recognize it as a
     subset of what a real device is running.
   - Cite the source in a comment at the top of the fixture so a future
     reader can trace it back.

4. `fboss/cli/fboss2/test/integration_test/Config<Area>Test.cpp` — inherit
   `Fboss2IntegrationTest`; for each attr read current value via
   `getRunningConfig()` thrift, set new value with `runCli()`, `commitConfig()`,
   verify, restore. See
   [ConfigArpTest.cpp](../../../fboss/cli/fboss2/test/integration_test/ConfigArpTest.cpp).
   - Prefer helpers that *derive* targets from the live running config
     (e.g. "find the first queue that has a `pktsPerSec` cap", "use the
     reason that already maps to something") rather than hardcoding
     IDs/names. The sample config tells you which shapes to expect, and
     the running config tells you which specific values this device has —
     basing the test on the latter keeps it portable across devices.

### Modified files

| File | Change |
|---|---|
| `fboss/cli/fboss2/CmdListConfig.cpp` | Add `#include` alphabetically; add `{"config", "<area>", "…", commandHandler<CmdConfig<Area>>, argRegistrar<CmdConfig<Area>Traits>}` to `kConfigCommandTree()` alphabetically. `argRegistrar<T>` dispatches to the Traits' `addCliArg` — do not use the legacy `argTypeHandler<T>` (that's the old `ObjectArgTypeId` path, still used only by not-yet-migrated non-config commands). |
| `cmake/CliFboss2.cmake` | Add the two new source files to `fboss2_config_lib` (alphabetically). |
| `fboss/cli/fboss2/BUCK` | Mirror the cmake additions into `srcs` and `headers` of `fboss2-config-lib`. |
| `cmake/CliFboss2TestConfig.cmake` | Add unit test `.cpp` (alphabetically). |
| `fboss/cli/fboss2/test/config/BUCK` | Mirror. |
| `cmake/CliFboss2TestIntegrationTest.cmake` | Add integration test `.cpp` (alphabetically). |
| `fboss/cli/fboss2/test/integration_test/BUCK` | Mirror. |

Keep the cmake and BUCK file lists in sync — the repo builds with more
than one build system, and a file added to only one of them breaks the
other.

## 5. Build + unit-test locally

Build, with whatever build system your environment uses, the following
targets:

- the `fboss2-dev` binary (`fboss/cli/fboss2`)
- the config unit tests (`fboss/cli/fboss2/test/config`, cmake target
  `fboss2_cmd_config_test`)
- the integration tests (`fboss/cli/fboss2/test/integration_test`, cmake
  target `fboss2_integration_test`)

Then run the unit tests, filtered to the new fixture:

```bash
<path-to>/fboss2_cmd_config_test --gtest_filter='CmdConfig<Area>TestFixture.*'
```

## 5a. Clean up includes (before committing)

**After** the build + tests pass, **before** committing, run your
environment's include hygiene tooling (e.g. clang's include-cleaner, IWYU)
over the files you added or changed — add direct includes where a symbol
was being picked up transitively, and remove redundant ones. Re-run the
build + tests afterward.

## 6. Integration testing on a device

Use a test switch provisioned with an FBOSS image built from your branch,
with the agent stack (`fboss_sw_agent`, `fboss_hw_agent@0`, `fsdb`,
`qsfp_service`) up and running. How you obtain and provision one is
environment-specific.

Sanity-check before running tests:

1. All FBOSS services are `active` under systemd.
2. The agent config exists and the agent has finished initializing.
3. `fboss2-dev show interface` succeeds without `Connection refused`.

Build + copy + run. **Two gotchas worth calling out up front**:

- The `config` commands live in the **`fboss2-dev`** binary, not `fboss2`.
  Build and deploy `fboss2-dev` — if you copy `fboss2` instead, the CLI
  rejects `config ...` with "The following arguments were not expected: ...".
- `scp` copies binaries over as mode 555. A second `scp` to the same path
  fails with `Permission denied` even for the owner. Use a fresh versioned
  filename rather than fighting the mode.

```bash
scp <path-to>/fboss2-dev <switch>:/tmp/fboss2-dev-<suffix>
scp <path-to>/fboss2_integration_test <switch>:/tmp/fboss2_it_<suffix>
ssh <switch> "/tmp/fboss2_it_<suffix> --gtest_filter='Config<Area>Test.*'"
```

Also capture an interactive session for the review's "Sample usage" — for
each attr run the CLI, then `config session diff`, then
`config session commit`, then show the relevant `show` output (or agent
running config) reflecting the new value.

## 6a. When the CLI surfaces an agent-side crash

Integration tests exercise code paths on the agent that may have latent
bugs. The CLI correctly producing a config delta that then crashes the
agent is a common outcome — the fboss_sw_agent and fboss_hw_agent@0
processes both auto-restart under systemd, so the device usually recovers
even though the test reports a failure.

**Find the real crash site**. The systemd core dump is typically
truncated (`coredumpctl info <pid>` shows "Storage: ... (truncated)")
and gdb without debug symbols is useless on it. Three better sources,
in order:

1. **`/var/facebook/logs/fboss/wedge_agent.log`** — the agent writes its
   full glog + the signal-handler's demangled stack trace here. Grep for
   `F<MMDD>|CHECK failed|Failed to|Terminated due to|SaiApiError`
   around the crash timestamp. This is the single most useful source —
   it includes the preceding error line (e.g.
   `[hash] Failed to remove sai object : HashSaiId(...): OBJECT IN USE`)
   which almost always names the faulty SAI call or CHECK.
2. **`sudo journalctl -u fboss_hw_agent@0 --since '<crash-time>'`** —
   useful for the process lifecycle (which process died first, when
   systemd restarted it) but the stack traces here are mostly unmangled
   thread frames with heavy idle/worker noise. Filter aggressively.
3. **gdb on the core** — `sudo coredumpctl debug <pid>
   --debugger-arguments='-batch -ex "thread 1" -ex "bt"'`. Only useful
   when the core isn't truncated.

In a hw/sw-agent split deployment, the **hw_agent crashes first**; the
sw_agent follows seconds later when it notices the hw_agent disconnected.
The sw_agent's stack trace shows an unrelated cleanup-path abort in
`HwSwitchConnectionStatusTable::disconnected` — ignore it and focus on
the hw_agent's crash.

## 6b. Fixing an agent-side bug in the same change

When the integration tests reveal an agent bug, decide scope:

- **Small mechanical fix (≲50 lines, clear precedent from a nearby
  working code path)**: fold it into the same commit. Example: a LAG
  hash delta crash was a missing clear-before-set in
  `SaiSwitchManager::addOrUpdateLagLoadBalancer` — the ECMP path already
  had the pattern and there was even a `TODO` flagging the gap. Ported
  in ~20 lines, tested on the same device.
- **Larger or uncertain fix**: keep the CLI change scoped, file a
  follow-up issue, disable or `GTEST_SKIP` the integration tests that
  reproduce the bug so CI doesn't core-dump the agent, and link the
  issue from the review.

If you include an agent fix, rebuild the hw_agent binary for your SAI
implementation, then redeploy:

```bash
scp <path-to>/fboss_hw_agent-sai_impl \
    <switch>:/tmp/fboss_hw_agent-sai_impl.<suffix>
ssh <switch> "sudo systemctl stop fboss_hw_agent@0 fboss_sw_agent && \
    sudo cp /tmp/fboss_hw_agent-sai_impl.<suffix> \
            /opt/fboss/bin/fboss_hw_agent-sai_impl && \
    sudo chmod +x /opt/fboss/bin/fboss_hw_agent-sai_impl && \
    sudo systemctl start fboss_hw_agent@0 fboss_sw_agent"
```

The binary is large so the scp can take a while on a management
network. Wait for the agent to come back up with a valid config before
re-running tests — the agent reports "switch is still initializing"
for the first few seconds:

```bash
until ssh <switch> "/tmp/fboss2-dev-<suffix> show interface 2>/dev/null | grep -q 'eth1/'"; do sleep 5; done
```

## 7. Commit + review

Write a commit message / review description with:

- **Summary** — what the commands do, action level, deferred pieces
- **Test Plan** — paste unit test output, integration test output, sample
  CLI transcript

If any design choice is deferred (e.g. a sub-attribute out of scope),
record *why* in the review description or the tracking issue so reviewers
don't have to reconstruct the reasoning.

## 8. Pitfalls observed

- **Forgetting `addCliArg` on the Traits (or wiring `argTypeHandler`
  instead of `argRegistrar` in `CmdListConfig.cpp`)** — the command tree
  registers fine but argument parsing silently no-ops.
  `BaseCommandTraits::addCliArg` is a no-op default, so a Traits that
  omits it compiles cleanly and just never receives its positional.
- **Parent "branch" node must have a handler for depth to increment**.
  If the parent of your leaf commands in `CmdListConfig.cpp` has no
  `commandHandler`, `addCommandBranch()` does not increment depth, so all
  leaves register their positional args at the same
  `CmdArgsLists::data_[0]` slot. Give the parent a trivial
  "Incomplete command" handler (see `CmdConfigCopp`, `CmdConfigL2`, or
  `CmdConfigQos` for the pattern) and set `ParentCmd = <ParentHandler>`
  on each leaf's Traits — otherwise the first argument of a multi-token
  positional gets dropped or the wrong slot gets read.
- **CLI11 subcommand fallthrough silently reclassifies positionals**
  when an argument value happens to match a subcommand name elsewhere in
  the tree. `config copp reason arp queue 0` broke because CLI11's
  `_valid_subcommand` recurses up the ancestor chain and matches "arp"
  against `config arp`, stealing it from reason's positional option. The
  fix is `->required()->expected(N)` on the `add_option(...)` chain in
  your Traits' `addCliArg`, which makes `_parse_subcommand()` route the
  token to `_parse_positional()` instead (CLI11 checks
  `_count_remaining_positionals(required=true) > 0` before doing the
  subcommand match). Apply whenever a positional arg's value space
  overlaps with subcommand names — especially protocol names like `arp`,
  `ndp`, `bgp`, `lldp` which are subcommands of `show`, `clear`, and
  `config`.
- **Using `session.saveConfig()` without args** when the action level is not
  HITLESS — the overload defaults to `AGENT` + `HITLESS`. Always pass the
  explicit service + level pair unless HITLESS is correct.
- **Missing `#include "fboss/cli/fboss2/CmdHandler.cpp"`** in the .cpp file
  — you'll get a link error about missing `CmdHandler<...>::run()`
  instantiation.
- **Testing `queryClient()`, not `printOutput()`** — unit tests should
  focus on the query/mutation side. Don't write tests that capture stdout
  from `printOutput`.
- **Device state assumptions** — don't assume a freshly imaged test switch
  is actually healthy. Always run through the §6 sanity checklist before
  trying to run integration tests.
- **Deploying `fboss2` instead of `fboss2-dev`** — only the `-dev` binary
  has the `config` subcommand tree. The parent binary rejects `config ...`
  with `The following arguments were not expected: ...`, which reads like a
  CLI parsing bug but is really the wrong target. Always build and copy
  the `fboss2-dev` output.
- **Device state leaks between integration-test runs** — tests capture the
  initial state as `originalFields` / `originalValue` for restore, but a
  crash-interrupted test run leaves the device in whatever the last
  successful commit produced, not the pre-test baseline. The next run
  captures *that* as "original" and a hardcoded restore string can then
  mismatch. Prefer deriving restore tokens from the captured state; if you
  hardcode them, know the first run after a failure may fail for
  test-setup reasons unrelated to the code under test (just re-run once
  the device settles).
- **Truncated core dumps are useless** — `coredumpctl info <pid>` showing
  `Storage: ... (truncated)` means gdb can't recover a real backtrace. Go
  straight to `/var/facebook/logs/fboss/wedge_agent.log` instead (see §6a).
- **sw_agent crash trail is a red herring** in the hw/sw-split deployment —
  when the hw_agent dies, the sw_agent aborts seconds later inside a
  cleanup path. Focus on the hw_agent crash; the sw_agent stack won't
  point at the real bug.
