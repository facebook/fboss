# pyre-strict

"""Combined Meta-side pre-flight CLI for the firmware-npi-onboard skill.

What this is
============
A single-binary entry point that runs every pre-flight check the
`firmware-npi-onboard` orchestrator skill needs before it starts driving
phases (AnR, AIM, FAM, Upick, Platform, Production-Tag) for a new BMC-Lite
FBOSS platform. The skill invokes this binary; CI also runs it against every
canonical spec on every diff to catch regressions early.

How it helps
============
Pre-flight is the gate. If any of these checks fail, the orchestrator must
stop — driving phases against an invalid spec would land broken configerator
diffs, malformed thrift API calls, or worse. By consolidating every check
behind one CLI:

  - The skill (and CI) has exactly one command to invoke and parse.
  - New checks added later (basset pool count, modelmaps lookup, conveyor
    pipeline existence, …) plug in here without changing the skill's
    contract or the CI test target.
  - Every error is prefixed with its check tier (`[schema]`, `[semantic]`)
    so the user can tell at a glance which layer surfaced the problem.

The checks run in a deliberate order:

  1. **Tier 1 — JSON Schema** (`spec_validator.validate_spec_dict`, structural).
     Types, required fields, enums, regex patterns. If this fails, the spec
     shape can't be trusted, so subsequent checks are short-circuited.

  2. **Tier 2 — Pydantic semantic** (same call, cross-field rules). Catches
     `instances` ↔ `pcie_address` shape consistency,
     `before_first_underscore` requires an underscore in the base name,
     post-expansion instance-name uniqueness across the spec.

  3. **Tier 3 — Cross-spec `shared_with_platforms`** (only when the configs
     root can be derived from the spec path). For each component, walks the
     `firmware.shared_with_platforms` list and checks every sibling spec
     declares a matching `platform_identifier` for the same component.
     Mismatches are hard failures (`[semantic]`); missing sibling specs are
     warnings (`[warn]`) so a not-yet-onboarded sibling doesn't block this
     spec.

The spec is the single source of truth for the orchestrator. Cross-checks
against external state (fw_util.json, modelmaps.thrift, basset pools) are
intentionally NOT in this CLI — the orchestrator's per-phase code consults
those at the moment of use.

When this is used
=================
  - **By the skill** when a user invokes `/firmware-npi-onboard <platform>`
    or asks Claude to onboard / validate a platform's firmware spec. The
    skill calls this binary, captures errors, reports them verbatim.
  - **By CI on every diff** via the `test_preflight` and
    `test_canonical_examples` python_unittest targets, so a schema or
    validator change that breaks an existing onboarded spec fails the diff
    before it lands.
  - **By engineers locally** as a quick check after editing a spec:
    `buck2 run fbcode//fboss/platform/firmware_onboarding/validator:run_preflight -- <path-to-spec>`

Exit codes
==========
  0 — every hard check passed (warnings may still be present on stderr).
  1 — one or more hard checks emitted errors. All messages go to stderr,
      prefixed with `[<check>]` so callers can tier them: `[schema]` and
      `[semantic]` are hard failures; `[warn]` is advisory.
  2 — usage error (wrong number of args, spec file not found, etc.).

Adding new pre-flight checks
============================
Add a new function to its own module that returns `list[str]` of prefixed
messages (e.g. `[depcheck] ...`), then refactor `run_preflight()` to call
both `validate_spec_file(...)` and the new function and concatenate the
returned lists before returning. (Today's `run_preflight()` returns the
result of `validate_spec_file()` directly — adding a second check is the
trigger for switching to a `messages: list[str] = []` accumulator pattern.)
Update `validator/BUCK` to add the new library to `:preflight`'s deps.
Tests for the new check go in `tests/test_<check>.py` with its own
`python_unittest` target.
"""

from __future__ import annotations

import sys
from collections.abc import Sequence
from pathlib import Path

from fboss.platform.firmware_onboarding.validator.spec_validator import (
    validate_spec_file,
)


def run_preflight(spec_path: Path, configs_root: Path | None = None) -> list[str]:
    """Run every pre-flight check against the spec at `spec_path`; return
    messages.

    Hard failures use `[schema]` / `[semantic]` prefixes; advisory issues
    use `[warn]`. Returns an empty list when every check passes.
    """
    return validate_spec_file(spec_path, configs_root=configs_root)


def _is_hard_failure(message: str) -> bool:
    return message.startswith(("[schema]", "[semantic]"))


def main(argv: Sequence[str] | None = None) -> int:
    """CLI entry point.

    Expects `argv` shaped like `sys.argv` (program name at index 0, args
    follow). When `argv is None`, `sys.argv` is used. To call programmatically
    with args only, pass `argv=["preflight", "spec.json"]`.
    """
    args = list(sys.argv if argv is None else argv)
    progname = args[0] if args else "preflight"
    if len(args) != 2:
        print(f"usage: {progname} <spec.json>", file=sys.stderr)
        return 2

    spec_path = Path(args[1]).resolve()
    if not spec_path.is_file():
        print(f"error: spec file not found: {spec_path}", file=sys.stderr)
        return 2

    # Cross-spec checks need the configs root: the directory holding
    # `<platform>/spec.json` for every onboarded platform. We only enable
    # the cross-spec check when the layout actually looks canonical —
    # checked by requiring at least one OTHER `<sibling>/spec.json` to
    # exist alongside this one. A bare relative path like `spec.json`
    # (resolved to `<cwd>/spec.json`) would otherwise enable the check
    # against arbitrary subdirectories of the cwd's parent, which is
    # almost never intentional.
    configs_root: Path | None = None
    grandparent = spec_path.parent.parent
    if grandparent != spec_path.parent and grandparent.is_dir():
        own_dirname = spec_path.parent.name
        if any(
            (child / "spec.json").is_file()
            for child in grandparent.iterdir()
            if child.is_dir() and child.name != own_dirname
        ):
            configs_root = grandparent

    # One-line breadcrumb so the caller can tell at a glance whether the
    # cross-spec `shared_with_platforms` check ran.
    if configs_root is not None:
        print(
            f"preflight: cross-spec checks enabled (configs_root={configs_root})",
            file=sys.stderr,
        )
    else:
        print(
            "preflight: cross-spec checks disabled (configs_root could not "
            "be derived from spec path)",
            file=sys.stderr,
        )

    # NOTE: validate_spec_file (called by run_preflight) already catches
    # json.JSONDecodeError / OSError internally and surfaces them as
    # `[schema]` messages — no local try/except needed here. A malformed
    # spec.json shows up as a normal `[schema]` hard failure with exit 1.
    messages = run_preflight(spec_path, configs_root=configs_root)
    hard = [m for m in messages if _is_hard_failure(m)]
    for msg in messages:
        print(msg, file=sys.stderr)
    if not hard:
        print("OK")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
