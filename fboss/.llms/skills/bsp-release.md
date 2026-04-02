---
description: Use when releasing a new BSP (Board Support Package) version for FBOSS platforms. Triggers on BSP release, release BSP, new BSP version, bspctl release, publish BSP, bump BSP version. Covers the full end-to-end process from build through production publish.
oncalls: ['fboss_platform']
---

# FBOSS BSP Release

End-to-end orchestration of the BSP release process. Stateless and discovery-based — can be invoked at any point in the release and will determine current progress by inspecting existing diffs and artifacts.

## Inputs

Required from user:
- **vendor or kmod_name**: either `arista` or `arista_bsp_kmods` — both are accepted, one derives the other. Supported vendors: `arista`, `cisco`, `fboss`.
- **version**: e.g. `0.7.21`

Prompted when needed:
- **hostname(s)**: target switch(es) for testing (prompted at test step)

Derived automatically:
- `kmod_name`: `<vendor>_bsp_kmods`
- `kmod_full_version`: `<version>-1`
- Source repo path: defaults from `bspctl` (e.g. `~/local/fboss.bsp.arista`)

## bspctl Location

All `bspctl` commands are run via:
```bash
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- <subcommand> <args>
```

## State Discovery

On every invocation, determine progress by checking these signals **in order**:

**Do not use date filters on any diff search** — the release may have started weeks ago.

### 1. Check for kernel-externals diff
Use the `diff-search` skill: search for diffs with filepath matching `<kmod_name>-<version>` (e.g. `arista_bsp_kmods-0.7.21`). This matches both the directory and `.sh` build script.
- Not found → Step 1 (build)
- Found, not landed → waiting on review

### 2. Check if SRPM is published
```bash
fbrpm get common/src <kmod_name>-<version>-1.src.rpm
```
Run in a temp directory. If downloadable → SRPM is published.

### 3. Check for configerator diff
Search for diffs with filepath `modules.cinc` AND title containing the kmod_name AND title containing the version (as separate `CONTAINS_ANY_TEXT` filters on `DIFF_TITLE`). The title format from `bspctl prepare-configerator` is `"Updated <kmod_name> to version <version>"`.
- Not found → need to run prepare-configerator
- Found, not landed → waiting on review

### 4. Check platform_manager.json files
Search `fboss/platform/configs/*/platform_manager.json` for files where `bspKmodsRpmName` matches `<kmod_name>`. List ALL matching platforms with their current `bspKmodsRpmVersion`.

### 5. Check for fbsource diff
Search for diffs with filepath `platform_manager.json` AND title containing the version string (e.g. `0.7.21`). The title format varies but always includes the version number.

### Status Display

Present a summary showing each step's status:
```
BSP Release: arista_bsp_kmods 0.7.21

  [done] Build complete (RPMs in ~/local/rpm/RPMS/)
  [done] kernel-externals diff D12345 — Landed
  [done] SRPM published
  [wait] configerator diff D67890 — Needs Review
  [    ] Publish RPMs
  [    ] Bump platform_manager.json (6/7 platforms on 0.7.21-1)
  [    ] Tag source repo
```

For the platform_manager.json step, always show per-platform version breakdown.

## Steps

### Step 1: Build

Check if RPMs already exist in `~/local/rpm/RPMS/` matching `<kmod_name>-*-<version>-1.x86_64.rpm`. If found, check their age using file modification time. Present findings to user:

```
Found existing RPMs for arista_bsp_kmods 0.7.21 (built 6 days ago, Mar 20):
  arista_bsp_kmods-6.4.3-0_fbk1_...-0.7.21-1.x86_64.rpm
  arista_bsp_kmods-6.11.1-0_fbk3_...-0.7.21-1.x86_64.rpm
  ...

These RPMs are 6 days old. Would you like to:
  1. Use existing RPMs (skip build)
  2. Rebuild from latest source
```

If no RPMs exist or user chooses to rebuild:
```bash
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  build <kmod_name> <version>
```

Builds for all kernel versions from configerator. RPMs saved to `~/local/rpm/RPMS/`.

Always show the user the recent commits from the source repo so they can verify the right code is included.

### Step 2: Test

Prompt user for target switch hostname(s) if not already provided. Run in background (tests take 30+ minutes with `--all-kernels`):

```bash
# Test on current kernel only (usually sufficient):
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  test <kmod_name> <version> <hostname>

# Test across all kernels (safer, but slower and requires kerctl):
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  test <kmod_name> <version> <hostname> --all-kernels
```

Ask the user whether to test on the current kernel or all kernels. Single-kernel is usually sufficient; all-kernels is safer but requires `kerctl install` to work and reboots the switch between kernels.

**If the test fails:**
- **Infrastructure issues** (timeouts, SSH failures, RPM lock contention, chef issues): retry the test. The switch may have been in a bad state.
- **BSP/kmod issues** (platform_manager crashes, missing kmods, driver errors): investigate the root cause before retrying.

### Step 3: Submit kernel-externals diff

**Before submitting**, verify the source repo is on `main` or `master`:
```bash
cd ~/local/fboss.bsp.<vendor> && git branch --show-current
```
If the source repo is on a feature branch or detached HEAD, **do not submit**. It's fine to build and test from any branch, but only release code that is on main/master. Alert the user and wait for them to merge their changes upstream first.

The build step created a branch `bspctl-<kmod_name>-<version>` in `~/local/kernel-externals`. Submit it:

```bash
cd ~/local/kernel-externals && jf submit --draft
```

**Summary:** List all new commits from the source repo since the last release. Find the previous version tag and diff against it:
```bash
cd ~/local/fboss.bsp.<vendor>
git tag -l | sort -V | tail -5          # find previous version tag
git log --oneline v<prev_version>..main  # commits since that tag
```
Use this commit list — do NOT use raw `git log` without a base tag, as it will include commits already in the previous release.

**Test plan:** Include the `bspctl test` output (platform_manager success output, bsp_tests results). Paste the output and reference it as `{<paste-number>}`.

Tell user: "kernel-externals diff needs review and landing before we can continue."

### Step 4: Publish SRPM

**Prerequisite:** kernel-externals diff must be landed (merged to master).

```bash
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  publish-srpm <kmod_name> <version>
```

This checks out master in kernel-externals, rebuilds the SRPM, signs it with `fbrpm`, and publishes via `svnyum`.

### Step 5: Prepare configerator

**Prerequisite:** SRPM must be published.

```bash
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  prepare-configerator <kmod_name> <version>
```

Downloads the published SRPM, computes sha256, updates `modules.cinc` and `sandcastle_rpms.cinc`, runs `arc build && arc canary`, and commits.

**Before submitting**, verify the RPM builds correctly in the linux repo. Check out any fb branch (the specific version doesn't matter):
```bash
cd ~/local/linux && git checkout v6.4-fb && arc build //:x86_64
```

Then submit:
```bash
cd ~/configerator && jf submit --draft
```

**Summary:** Reference the kernel-externals source diff (e.g. D12345) and confirm the SRPM has been published.

**Test plan:** Include the output of `arc build //:x86_64` from the linux repo. Paste the output and reference it as `{<paste-number>}`.

Tell user: "configerator diff needs review and landing before we can continue."

### Step 6: Publish RPMs

**Prerequisite:** configerator diff must be landed.

```bash
buck2 run fbcode//fboss/util/facebook/platform_utils:bspctl -- \
  publish-rpms <kmod_name> <version>
```

Kicks off `kerctl standalone-module` builds for all kernel versions, waits for Sandcastle jobs, collects everstore handles, and publishes via `kerctl publish`.

### Step 7: Bump platform_manager.json

**Prerequisite:** RPMs must be published.

Find all `fboss/platform/configs/*/platform_manager.json` files where `bspKmodsRpmName` equals `<kmod_name>`. Present a table to the user:

```
Platforms using arista_bsp_kmods:

  Platform              Current Version    Action
  icecube800banw        0.7.20-1           Already up to date
  meru800bia            0.7.20-1           Already up to date
  darwin                0.7.9-1            Update to 0.7.21-1?
  ...
```

Ask the user which platforms to update. Accept:
- `all` — update every platform that is not already on the target version
- A list of specific platform names
- `none` / skip — if the user wants to handle this separately

For selected platforms, update `bspKmodsRpmVersion` to `<version>-1`.

Create a commit in fbsource and submit:
```bash
jf submit --draft
```

**Test plan:** Run platform_manager on a switch with the new BSP version installed and include the output showing successful startup. Paste the output and reference it as `{<paste-number>}`.

Tell user: "fbsource diff needs review and landing."

### Step 8: Tag source repo

**Prerequisite:** platform_manager.json diff landed.

Prompt user to tag the source BSP repo. Check existing tag format first (`git tag -l | tail -5`) to match the convention (e.g. `v0.7.21` vs `0.7.21`):
```bash
cd ~/local/fboss.bsp.<vendor>
git tag v<version>
git push origin v<version>
```

Print: "BSP release complete!"

## Async Gates

Three points where the skill must pause and wait for human action:

1. **After Step 3** — kernel-externals diff must be approved and landed
2. **After Step 5** — configerator diff must be approved and landed
3. **After Step 7** — fbsource diff must be approved and landed

At each gate, display the diff link and instruct the user to re-invoke the skill once the diff has landed.

## Troubleshooting

### RPM build fails
- Run `buck clean` in `~/local/linux` and retry
- Verify configerator changes are canaried
- Check kmod name/version spelling matches exactly

### SRPM publish fails (ACL error)
- Request access to groups: `fboss_sdk_rpms` and `oncall_fboss_platform`
- Ensure manual_sign is enabled for fboss_platform oncall

### kerctl standalone-module fails
- Verify configerator diff is landed (not just submitted)
- Check the Sandcastle job URL for detailed errors
- Can resume with `--job-ids` flag if some jobs succeeded

### platform_manager won't start with new BSP
- Check `rpm -qa | grep <kmod_name>` to verify RPM installed
- Check kernel version matches: `uname -r`
- Review kmods.json for missing modules
