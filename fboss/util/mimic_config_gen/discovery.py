# Copyright (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

"""Resolve the donor 2x2 from netwhoami.

The caller always names role, source hardware and target hardware, so nothing
here chooses hardware. What it does resolve is (a) a concrete, healthy source
device when one was not pinned, and (b) the bridge role -- some role the target
hardware already serves, which is the only place its platform artifacts exist.
"""

from __future__ import annotations

import json
import logging
import subprocess

from fboss.util.mimic_config_gen.defs import Donors, MimicError

logger: logging.Logger = logging.getLogger(__name__)

NETWHOAMI_BIN = "netwhoami"

# A device we are willing to clone from. Anything draining, in a lab, or
# mid-maintenance risks a GSC that does not represent steady state.
_HEALTHY_TERMS = (
    "serf_state=IN_USE",
    "maintenance_state=NONE",
    "fbnet_state=IN_USE",
)
_LAB_TAG = "LAB_DEVICE"


def _netwhoami(args: list[str]) -> str:
    cmd = [NETWHOAMI_BIN, *args]
    logger.debug("running %s", cmd)
    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=180, check=False
        )
    except FileNotFoundError as e:
        raise MimicError(
            f"{NETWHOAMI_BIN} not found on PATH; this tool must run on a devserver"
        ) from e
    except subprocess.TimeoutExpired as e:
        raise MimicError(f"{NETWHOAMI_BIN} timed out: {' '.join(cmd)}") from e
    if proc.returncode != 0:
        raise MimicError(
            f"{' '.join(cmd)} failed ({proc.returncode}):\n{proc.stderr.strip()}"
        )
    return proc.stdout


def _counts(scope: str, group_by: str) -> dict[str, int]:
    """Return {group_value: device_count} for a netwhoami scope.

    Empty group keys are dropped: netwhoami emits them for nullable fields
    (`tags=: 509923`), and an empty key would otherwise be selectable as a
    bridge role and fail much later inside an enum lookup.
    """
    out = _netwhoami(["counts", scope, "--group-by", group_by])
    counts: dict[str, int] = {}
    saw_output = False
    for line in out.splitlines():
        line = line.strip()
        if not line:
            continue
        saw_output = True
        if "=" not in line or ":" not in line:
            continue
        key, _, num = line.rpartition(":")
        if not key.startswith(f"{group_by}="):
            continue
        value = key.split("=", 1)[1]
        try:
            parsed = int(num.strip())
        except ValueError:
            continue
        if value:
            counts[value] = parsed
    if saw_output and not counts:
        # Every line was unparseable. Far better to say so than to report an
        # empty fleet, which callers turn into confident "no such hardware"
        # diagnoses.
        raise MimicError(
            f"could not parse any '{group_by}=' counts from netwhoami; its "
            f"output format may have changed. Scope: {scope}"
        )
    return counts


def roles_for_hw(hw: str) -> dict[str, int]:
    return _counts(f"hw={hw}", "role")


def hw_for_role(role: str) -> dict[str, int]:
    return _counts(f"role={role}", "hw")


def pick_bridge_role(
    target_hw: str,
    source_hw: str,
    role: str,
    pinned: str | None = None,
) -> tuple[str, dict[str, int], list[str]]:
    """Choose the role that will donate target-hw platform artifacts.

    Preference order:
      1. a role served by BOTH target and source hardware -- that gives us a
         pivot cohort, which is what lets us tell hardware-shaped inputs from
         role-shaped ones instead of guessing
      2. failing that, any role the target hardware serves

    Within a tier we take the largest target-hardware population, breaking ties
    by name so repeated runs pick the same bridge role: the choice changes the
    donor classification and therefore the generated config.

    Returns (bridge role, roles the target hardware serves, warnings to report).
    """
    warnings: list[str] = []
    target_roles = roles_for_hw(target_hw)
    # TEST devices carry deliberately unrepresentative config.
    target_roles.pop("TEST", None)
    # The role being generated cannot also be the bridge: that makes the hw
    # donor identical to the target, so nothing is ever seen as missing and the
    # tool reports a clean run while emitting the source config with a forged
    # hardware field.
    target_roles.pop(role, None)
    if not target_roles:
        raise MimicError(
            f"{target_hw} serves no role other than {role} in netwhoami, so "
            "there is no cohort to donate its platform artifacts. A brand-new "
            "hardware needs its configerator inputs authored first."
        )

    source_roles = set(roles_for_hw(source_hw))
    if pinned:
        pinned_up = pinned.upper()
        if pinned_up not in target_roles:
            raise MimicError(
                f"--bridge-role {pinned} invalid: {target_hw} serves "
                f"{sorted(target_roles)} (excluding the target role {role})"
            )
        if pinned_up not in source_roles:
            warnings.append(
                f"bridge role {pinned_up} is not served by {source_hw}, so "
                "there is no pivot cohort; every input will be classified "
                "role-shaped rather than measured"
            )
        return pinned_up, target_roles, warnings

    shared = {r: n for r, n in target_roles.items() if r in source_roles}
    tier = shared or target_roles
    if not shared:
        warnings.append(
            f"no role is served by both {target_hw} and {source_hw}, so there "
            "is no pivot cohort; every input will be classified role-shaped "
            "rather than measured"
        )
    chosen = min(tier.items(), key=lambda kv: (-kv[1], kv[0]))[0]
    return chosen, target_roles, warnings


def pick_source_device(role: str, source_hw: str) -> str:
    """Pick a healthy production device from the caller-named cohort."""
    # Exclude lab devices server-side rather than by substring on the returned
    # tags cell -- netwhoami supports the term, and it keeps the response small.
    scope = ",".join(
        [f"role={role}", f"hw={source_hw}", f"tags!={_LAB_TAG}", *_HEALTHY_TERMS]
    )
    out = _netwhoami(["fields", scope, "--fields", "name"])

    candidates = [
        line.split("\t")[0].strip()
        for line in out.splitlines()
        if line.split("\t")[0].strip()
    ]

    if not candidates:
        raise MimicError(
            f"no healthy production device found for {role.lower()}/"
            f"{source_hw.lower()} (scope: {scope}, excluding {_LAB_TAG}). "
            "Pass --source-device explicitly if you know one."
        )
    # Sorted so repeated runs pick the same device and stay reproducible.
    return sorted(candidates)[0]


def resolve(
    role: str | None,
    source_hw: str | None,
    target_hw: str,
    source_device: str | None,
    bridge_role: str | None,
) -> tuple[Donors, dict[str, int], list[str]]:
    """Work out the donor 2x2.

    A named source device already states its own role and hardware, so those
    arguments become optional. When both are supplied they must agree with the
    device -- a silent override would generate a config for a pair the caller
    did not ask for.

    Returns (donors, roles the target hardware serves, warnings to report).
    Warnings are returned rather than logged so they land in the report on
    stdout next to the claims they qualify, not on stderr where they are easy
    to miss.
    """
    target_hw = target_hw.upper()
    role = role.upper() if role else None
    source_hw = source_hw.upper() if source_hw else None

    if source_device:
        dev_role, dev_hw = device_identity(source_device)
        for label, given, actual in (
            ("--role", role, dev_role),
            ("--source-hw", source_hw, dev_hw),
        ):
            if given and given != actual:
                raise MimicError(
                    f"{label}={given} contradicts {source_device}, which is "
                    f"{dev_role.lower()}/{dev_hw.lower()}. Drop {label} to take "
                    "it from the device."
                )
        role, source_hw = dev_role, dev_hw
    elif not (role and source_hw):
        raise MimicError(
            "supply --source-device, or both --role and --source-hw so a "
            "source device can be chosen"
        )

    if source_hw == target_hw:
        raise MimicError("--source-hw and --target-hw must differ")

    # Validate against the thrift enums before any scope string reaches the
    # netwhoami CLI: an unrecognised value there exits non-zero and files a
    # LogView task against another team's oncall.
    _validate_enums(role, source_hw, target_hw)

    warnings: list[str] = []
    existing = hw_for_role(role)
    if source_hw not in existing:
        raise MimicError(
            f"{source_hw} does not serve role {role}; there is nothing to clone. "
            f"{role} runs on {sorted(existing)}"
        )
    if target_hw in existing:
        warnings.append(
            f"{target_hw} already serves {role} ({existing[target_hw]} devices) "
            "-- this pair is not new, so a plain coop_test run against a real "
            "device is probably what you want"
        )

    chosen_bridge, target_roles, bridge_warnings = pick_bridge_role(
        target_hw, source_hw, role, bridge_role
    )
    warnings.extend(bridge_warnings)
    device = source_device or pick_source_device(role, source_hw)

    return (
        Donors(
            role=role,
            source_hw=source_hw,
            target_hw=target_hw,
            bridge_role=chosen_bridge,
            source_device=device,
        ),
        target_roles,
        warnings,
    )


def _validate_enums(role: str, source_hw: str, target_hw: str) -> None:
    from netwhoami import thrift_types as tt

    for label, value, enum in (
        ("--role", role, tt.Role),
        ("--source-hw", source_hw, tt.Hardware),
        ("--target-hw", target_hw, tt.Hardware),
    ):
        if getattr(enum, value, None) is None:
            raise MimicError(f"{label}={value} is not a known netwhoami value")


def device_identity(device: str) -> tuple[str, str]:
    """Return (role, hw) for a real device, straight from netwhoami."""
    out = _netwhoami(["host", device])
    start = out.find("{")
    if start < 0:
        raise MimicError(f"netwhoami returned no record for {device}")
    try:
        record = json.loads(out[start:])
    except json.JSONDecodeError as e:
        raise MimicError(f"could not parse netwhoami record for {device}: {e}") from e

    role, hw = record.get("role"), record.get("hw")
    if not role or not hw:
        raise MimicError(f"{device} has no role/hw in netwhoami (role={role}, hw={hw})")
    return str(role).upper(), str(hw).upper()


def asic_profile(hw: str) -> tuple[str | None, str | None, list[str]]:
    """Return (asic, asic_vendor, warnings) for a hardware, as the fleet reports it.

    Read from netwhoami rather than hardcoded, so new hardware needs no code
    change. Generator code branches on `asic` directly -- not just via PolicyDB
    scopes -- so the forged identity has to carry it, not only `hw`.

    Some hardware reports several ASICs (ARISTA_7808 spans JERICHO2,
    JERICHO2PLUS and JERICHO3). Picking the most populous silently decides
    which variant the config is generated for, so say so rather than guess
    quietly.
    """
    warnings: list[str] = []
    asics = _counts(f"hw={hw}", "asic")
    vendors = _counts(f"hw={hw}", "asic_vendor")

    def top(counts: dict[str, int], field: str) -> str | None:
        if not counts:
            warnings.append(
                f"netwhoami records no {field} for {hw}; the forged identity "
                f"will carry no {field}"
            )
            return None
        # Deterministic: highest population, ties broken by name.
        chosen = min(counts.items(), key=lambda kv: (-kv[1], kv[0]))[0]
        if len(counts) > 1:
            others = ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))
            warnings.append(
                f"{hw} reports multiple {field} values ({others}); generating "
                f"for {chosen}. Config correctness depends on this being the "
                "right variant."
            )
        return chosen

    return top(asics, "asic"), top(vendors, "asic_vendor"), warnings
