# Vendor Escalation Package

## When to Use

When a test is categorized **`FAIL_VENDOR`** (or `PASS_VENDOR_FIX`) — i.e. the evidence
points at the vendor SAI/SDK or silicon, not FBOSS — assemble a self-contained package the
vendor can reproduce and debug **without access to FBOSS**. This applies to any SAI vendor:
**Broadcom** (CSP case), **Cisco/Leaba** (TAC case), **NVIDIA** (support case).

A good package lets the vendor replay the exact SAI call sequence on their own setup and see
the same failure. It contains three things:

1. **SAI Replayer log(s)** — the exact, vendor-agnostic sequence of SAI calls FBOSS made.
2. **Hardware init config file(s)** — the per-ASIC config used at `create_switch`.
3. **A written case report** — environment, setup/steps, what's broken, initial debugging,
   and why it points to the vendor SDK.

## Why these artifacts prove it is a vendor issue

- The **SAI Replayer log** is a compilable C/C++ program that re-issues the identical SAI
  API calls (same attributes, same order) the agent made. It is **vendor-neutral** — it only
  contains the SAI interface, no FBOSS logic. If the vendor replays it and reproduces the
  failure, the defect is provably below SAI (in the vendor adapter/SDK/silicon), not in FBOSS.
- The **hw_config** is the vendor's own SOC/init configuration consumed at `create_switch`.
  Pairing it with the replayer makes the bring-up bit-for-bit reproducible.
- The strongest evidence is a **differential (A/B)**: the *same* SAI sequence and *same*
  hw_config (modulo one controlled variable — e.g. a single switch attribute, port, or
  ASIC instance) that works in one case and fails in the other. See
  [Differential method](#differential-ab-method-strongest-evidence).

## Enabling the artifacts (flags)

All of these are flags on the **process that owns the SAI layer**:
- **mono** (`sai_agent_hw_test`): flags go on the test binary.
- **multi** (`multi_switch_agent_hw_test`): flags go on each **`fboss_hw_agent`** process
  (one per switchIndex), **not** the test binary.

| Flag | Required? | Purpose |
|------|-----------|---------|
| `--enable_replayer` | **Yes** | Turn on the SAI function wrapper + logger (writes the replayer log). |
| `--sai_log <path>` | **Yes** | Output path for the replayer log. In multi, give each hw_agent a distinct path (per switchIndex). |
| `--enable_packet_log` | Optional | **Also log the SAI calls that send packets** (`send_hostif_packet`). Enable when the repro involves TX/forwarding (e.g. a forwarding/rewrite/trap test). **Caveat:** tests that send many packets can bloat the log — enable only for low-packet repros. |
| `--enable_get_attr_log` | Optional | Also log SAI *get*-attribute calls (what the agent reads back). Useful when the suspected bug is in values returned by the SDK. |
| `--enable_sai_log DEBUG` | Optional | Vendor SDK internal verbose logging (separate from the replayer; goes to stdout / `/var/log/messages`). Attach if it shows the SDK's internal decision. |

> Decision: always `--enable_replayer --sai_log`. Add `--enable_packet_log` **if the repro
> is about packet forwarding/rewrite/trap**. Add `--enable_get_attr_log` if the suspected
> bug is in read-back values. See [enable-logging.md](enable-logging.md) for the full set of
> logging options.

## hw_config file location

FBOSS writes the create_switch config to `/dev/shm/fboss/` early in init (before the long
SDK/ASIC bring-up), so it is available shortly after the hw_agent starts:

| Mode | Path(s) |
|------|---------|
| mono | `/dev/shm/fboss/hw_config` |
| multi | `/dev/shm/fboss/hw_config_idx0`, `/dev/shm/fboss/hw_config_idx1`, … (one per switchIndex) |

**Capture it early.** It can be removed on test teardown — snapshot it as soon as it
appears (the collection script does this), not after the test exits.

## Collecting the package

Use `scripts/collect_vendor_escalation.sh`. Upload it to the switch and run it in the
background (see the device-access reference in the SKILL's Reference Routing for your
environment's upload / background-run commands). It runs the test with the replayer enabled,
snapshots the hw_config early, flushes the replayer log on graceful stop, and assembles named
outputs in an output directory.

```bash
# Usage:
#   collect_vendor_escalation.sh <mono|multi> <hw_agent_or_-> <test_binary> <config> \
#       <gtest_filter> <switch_id> <out_dir> <pkt_log:0|1> [get_attr_log:0|1] [suffix]

# Broadcom multi-switch example (capture switch_id 4, log packets):
bash /tmp/collect_vendor_escalation.sh multi \
  /root/<user>/fboss_hw_agent-brcm-<ver>_dnx_odp \
  /root/<user>/multi_switch_agent_hw_test \
  /root/<user>/<platform>_multi.agent.materialized_JSON \
  AgentMgmtPortL3ForwardingTest.VerifyMacRewrite \
  4 /root/<user>/escalation 1 0 sid4

# mono example (single ASIC):
bash /tmp/collect_vendor_escalation.sh mono - \
  /root/<user>/sai_agent_hw_test-brcm-<ver>_dnx_odp \
  /root/<user>/<platform>.agent.materialized_JSON \
  AgentSomeTest.SomeCase \
  0 /root/<user>/escalation 1 0 run0
```

For an **A/B** package, run it twice with the controlled variable changed (e.g. two
switch_ids, or two ports), collect both, then download all artifacts.

### Manual equivalent (if not using the script)

- Add the flags above to the SAI-owning process; give each switchIndex a distinct
  `--sai_log` path.
- `cp /dev/shm/fboss/hw_config_idx0 <out>/hw_config_idx0` (and `_idx1`) **right after**
  the files appear at boot.
- Let the test run to completion (it programs and sends the packet even if it asserts-fail);
  then stop the hw_agents with **SIGTERM** (graceful) so the replayer log is flushed before
  copying.

## Differential (A/B) method — strongest evidence

When the same code/config works in one case and fails in another (different switch_id, port,
ASIC instance, or one attribute), capture **both** and diff them. This isolates the single
responsible variable and is the most persuasive vendor evidence.

1. Collect replayer + hw_config for the **working** case and the **failing** case.
2. **hw_config**: diff, then exclude expected per-instance noise (e.g. SerDes lane maps,
   polarity flips) to see if any *functional* property differs.
   ```bash
   diff <(grep -vE 'lane_to_serdes_map|polarity_flip' hw_config_A | sort) \
        <(grep -vE 'lane_to_serdes_map|polarity_flip' hw_config_B | sort)
   ```
3. **Replayer logs**: confirm same length / same call counts (structural identity), then
   diff and confirm the only semantic differences are the controlled variable + expected
   per-instance values (object handles/OIDs, IP/MAC of the specific entity, file paths,
   timestamps).
   ```bash
   wc -l replayer_A replayer_B
   grep -c create_route_entry replayer_A replayer_B   # repeat per call type
   diff replayer_A replayer_B
   ```
4. State the conclusion: "identical SAI programming + identical functional config; only
   `<variable>` differs; behavior differs ⇒ defect is below SAI."

## Case report template

Save as `CASE_writeup.md` next to the artifacts. Keep it vendor-neutral and fact-based.

```markdown
# <Platform> / <ASIC>: <one-line symptom>

## Summary
<2-4 sentences: what is broken, on what, the single controlled variable, and that it
follows that variable (not the physical HW / SDK version).>

## Environment
- Platform / ASIC:
- SAI version / SDK version / SAI adapter version  (and any other versions it reproduces on)
- Mode (mono/multi, VOQ/DSF, switch_ids):

## Test / setup (steps)
<Exact objects programmed (RIF/neighbor/route/next-hop/ACL...) with key values, and the
stimulus (packet sent: how, dst IP/MAC, pipeline-lookup vs direct).>

## Expected vs Observed
<Table. Include the actual observed bad behavior, e.g. dst MAC not rewritten / packet looped
/ drop counter / wrong egress.>

## Analysis / what we verified
- SAI programming identical between A and B except <variable>  (replayer diff).
- hw_config functionally identical (only serdes/polarity differ).
- Failure follows <logical variable>, not physical HW  (state any swap experiment).
- Reproduces on SDK versions: <...>.
- Scope: <what works vs what fails — e.g. only mgmt port, regular front-panel ports fine>.

## Attachments (with md5)
<list the replayer logs + hw_config files>

## What we need from <vendor>
1. Root cause of <symptom> under <variable>.
2. Any constraints / required handling for <variable>.
3. Correct vendor diag command(s)/tables to inspect <relevant HW object>.
4. Fix / workaround.

## How to reproduce
<how to replay the attached logs / minimal steps>
```

## Per-vendor specifics

The replayer log and the package methodology are **identical across vendors** (SAI is the
common interface). Only the file particulars and the escalation channel differ:

| | Broadcom | Cisco / Leaba | NVIDIA |
|---|---|---|---|
| ASIC examples | Jericho3/Ramon (DNX), Tomahawk (XGS) | Leaba/Cisco Silicon One | Spectrum |
| hw_config content | SOC property config (`*.BCM<chip>=...`) | Leaba JSON/SDK config | Spectrum config |
| Firmware/SDK prereq on switch | DNX `db/` firmware at `/tmp/db/` — see [build-and-load.md](build-and-load.md) | Leaba SDK libs/res — see your env's build reference | per NVIDIA SDK |
| Escalation channel | Broadcom **CSP** case | Cisco **TAC** case | NVIDIA **support** case |
| Diag shell | `bcm` / DNX diag, `bcmsai`, `dbal` | Leaba CLI | NVIDIA diag |

The SAI Replayer log + hw_config + case report are what every vendor needs; attach them to
the relevant channel.

## Worked example (Broadcom)

Symptom: on a dual-NPU Jericho3-AI (Janga800BIC), an L3 route egressing the **management
port** is not getting its dst-MAC rewritten when the SAI switch is created with
`SAI_SWITCH_ATTR_SWITCH_ID = 4`; the peer switch (`SWITCH_ID = 0`) rewrites correctly, and
on the same failing NPU **regular front-panel ports work**. Collected per-NPU
`sai_replayer_npu{0,1}.log` (with `--enable_packet_log`) + `hw_config_idx{0,1}`; the replayer
diff showed identical programming except `SWITCH_ID` (0 vs 4) and the next-hop's own IP, and
hw_config differed only in SerDes/polarity — i.e. identical SAI + functional config, divergent
behavior by `SWITCH_ID` ⇒ filed to Broadcom CSP as a non-zero-`SWITCH_ID` SDK/silicon defect.

## Checklist before filing

- [ ] Replayer log captured for the failing case (and the working case if A/B).
- [ ] `--enable_packet_log` used **iff** the repro involves packet TX/forwarding.
- [ ] hw_config snapshotted (per switchIndex) and not empty.
- [ ] md5 of every attachment recorded.
- [ ] Case report filled (env, steps, expected/observed, analysis, asks).
- [ ] Stated clearly why it is below SAI (replayer reproduces it; FBOSS programming identical).

## Next Steps

- Categorize the test `FAIL_VENDOR` (see [test-categorization.md](test-categorization.md)).
- File the package to the vendor channel from the per-vendor table.
- If a vendor diag dump is also requested, see [vendor-diag-shell.md](vendor-diag-shell.md).
