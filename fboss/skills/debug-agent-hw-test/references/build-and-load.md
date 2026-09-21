# Build and Load Test Binaries (Meta External)

## When to Use

When you need to build an AgentHwTest binary (mono or multi-switch) and deploy it to a switch for testing.

## Inputs Required

| Input | Example | Required? |
|-------|---------|-----------|
| Mode | `mono` or `multi` | Yes |
| SDK version | `12.2 DNX`, `14.0 ea dnx`, `leaba 24.8.3001` | Yes |
| Switch name | `rsw1ab.p001.f01.snc1` | Yes (for copy/load) |

## Config Files

The test binary needs a platform-specific agent config file.

### Naming Convention

| Mode / Role | Config file name pattern | Example |
|-------------|-------------------------|---------|
| Mono | `<platform>.agent.materialized_JSON` | `meru800bfa.agent.materialized_JSON` |
| Multi | `<platform>_multi.agent.materialized_JSON` | `meru800bfa_multi.agent.materialized_JSON` |
| Hyper port | `<platform>_hyperport.agent.materialized_JSON` | `meru800bia_hyperport.agent.materialized_JSON` |
| Dual-stage DSF FAP (EDSW 3q/2q) | `<platform>_fap_edsw_3q_2q.agent.materialized_JSON` | `meru800bia_fap_edsw_3q_2q.agent.materialized_JSON` |
| Dual-stage DSF FAP (RDSW 3q/2q) | `<platform>_fap_rdsw_3q_2q.agent.materialized_JSON` | `meru800bia_fap_rdsw_3q_2q.agent.materialized_JSON` |
| Dual-stage DSF FE2 | `<platform>_fe2.agent.materialized_JSON` | `meru800bfa_fe2.agent.materialized_JSON` |
| Dual-stage DSF FE13 | `<platform>_fe13.agent.materialized_JSON` | `meru800bfa_fe13.agent.materialized_JSON` |

When the user specifies a test scenario (e.g., "hyper port mode", "dual stage FE2"), select the matching config variant from the table above.

### Copy Config to Switch

Upload the config file to the switch:

    UPLOAD <local-config-path> TO <switch>:/root/<user>/<config-file>

See the resolved device-access reference selected via the Reference
Routing table in `SKILL.md` for the upload command in your
environment.

> **Note**: In multi-switch mode with `--hw_agent_for_testing`, the hw_agent processes ignore the `--config` flag entirely. They wait for the test binary to provide a config file via a shared file mechanism (`hw_agent_test_*.conf`). The `--config` flag is only consumed by the test binary itself. This means using a mono config in place of a multi config may work in practice, but using the correct `_multi` config is recommended.

### Config File Location

See the resolved build-environment reference selected via the
Reference Routing table in `SKILL.md` for where config files are
stored in your environment.

## Build

See the resolved build-environment reference selected via the
Reference Routing table in `SKILL.md` for build commands, output
paths, and SDK locations specific to your environment.

### Mono (sai_agent_hw_test)

Build a single `sai_agent_hw_test` binary linked against your SAI SDK.

### Multi-switch (multi_switch_agent_hw_test + fboss_hw_agent)

Multi-switch mode requires **two** binaries:
- `multi_switch_agent_hw_test` — the test binary (SDK-independent)
- `fboss_hw_agent-<sdk-version>` — the hw_agent binary (SDK-specific)

## Strip and Copy to Switch

Two-step process: (1) strip locally, (2) upload to switch with md5 dedup.

Keep the full versioned binary name (e.g., `sai_agent_hw_test-brcm-12.2.0.0_dnx_odp`) so you always know which SDK version is deployed.

### Step 1: Strip locally

```bash
bash scripts/strip_and_copy.sh \
  <source_binary_path> \
  <dest_binary_name>
```

This creates `/tmp/<dest_binary_name>` (stripped) and prints its md5.

### Step 2: Upload to switch with md5 dedup

Check the md5 of the binary at its final destination (`/root/<user>/`), then upload if different:

    RUN ON <switch>: mkdir -p /root/<user> && md5sum /root/<user>/<dest_binary_name> 2>/dev/null || echo MISSING

If md5 differs or MISSING:

    UPLOAD /tmp/<dest_binary_name> TO <switch>:/tmp/<dest_binary_name>
    RUN ON <switch>: mv /tmp/<dest_binary_name> /root/<user>/<dest_binary_name> && chmod +x /root/<user>/<dest_binary_name>

See the resolved device-access reference selected via the Reference
Routing table in `SKILL.md` for the actual commands in your
environment.

> **Note**: If you need to debug crashes with GDB, see [crash-debug.md](crash-debug.md) — that workflow uses the non-stripped binary instead.

## Leaba/Cisco SDK Libraries

For Leaba (Cisco) ASIC targets (e.g., gibraltar, pacific), you must also copy the `res` and `lib` directories from the Leaba SDK to the switch. These may be **symlinks** — use `tar -h` to follow them so that real files (not dangling symlinks) end up on the switch.

See the resolved build-environment reference selected via the
Reference Routing table in `SKILL.md` for where to find the Leaba SDK
`res/` and `lib/` directories and any helper scripts for your
environment.

Once you have the paths, create tarballs and upload:

```bash
# Local: create tarballs (follow symlinks)
tar czfh /tmp/leaba_res.tar.gz -C <path-containing-res-dir> res
tar czfh /tmp/leaba_lib.tar.gz -C <path-containing-lib-dir> lib
```

Then upload and extract on the switch:

    UPLOAD /tmp/leaba_res.tar.gz TO <switch>:/tmp/leaba_res.tar.gz
    UPLOAD /tmp/leaba_lib.tar.gz TO <switch>:/tmp/leaba_lib.tar.gz
    RUN ON <switch>: mkdir -p /root/<user> && tar xzf /tmp/leaba_res.tar.gz -C /root/<user>/ && tar xzf /tmp/leaba_lib.tar.gz -C /root/<user>/

## Broadcom DNX Firmware (Required for All Tests on DNX Platforms)

**Applies to**: Broadcom DNX platforms only (Jericho3, Ramon3, and other DNX ASICs — e.g., Janga800BIC, Meru800BFA). Does **not** apply to Broadcom XGS platforms or Leaba/Cisco platforms.

All tests running on Broadcom DNX switches require the firmware `db/` directory at `/tmp/db/` on the switch. Without it, the hw_agent processes abort during initialization with:

```
FW: /tmp/db/jericho3ai_a0/fi-2.4.11-GA.elf is not accessible error:-1
```

See the resolved build-environment reference selected via the
Reference Routing table in `SKILL.md` for where to find the firmware
`db/` directory and any helper scripts for your environment.

Once you have the path, create a tarball and upload:

```bash
# Local: create tarball
tar czf /tmp/bcm_firmware_db.tar.gz -C <path-containing-db-dir> db
```

Then check, upload, and extract on the switch:

    RUN ON <switch>: ls /tmp/db/jericho3ai_a0/fi-*.elf 2>/dev/null | wc -l

If firmware is not present:

    UPLOAD /tmp/bcm_firmware_db.tar.gz TO <switch>:/tmp/bcm_firmware_db.tar.gz
    RUN ON <switch>: tar xzf /tmp/bcm_firmware_db.tar.gz -C /tmp/

Verify:

    RUN ON <switch>: ls /tmp/db/jericho3ai_a0/fi-*.elf

**When to run**: Before running any test on a Broadcom DNX switch. Not needed for non-DNX platforms.

## Verify on Switch

    RUN ON <switch>: ls -la /root/<user>/<binary-name>

## Next Steps

After loading the binary, proceed to [run-tests.md](run-tests.md) to execute the test.
