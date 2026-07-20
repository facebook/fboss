# pyre-strict

"""Two-tier validator for the FBOSS Platform Firmware Onboarding spec.

Tier 1 (structural)  — JSON Schema at ../schemas/platform-spec.v1.json,
                       enforced via the `jsonschema` package.
Tier 2 (semantic)    — Pydantic v2 models with cross-field validators below.

Tier 1 catches shape / type / enum / regex errors. Tier 2 catches the
relationships JSON Schema cannot express cleanly: `instances` ↔ `pcie_address`
shape consistency, `before_first_underscore` requires an underscore in the
component name, and post-expansion instance-name uniqueness across the spec.

Optional cross-spec check (only when `configs_root` is supplied to
`validate_spec_file`): walks each component's `firmware.shared_with_platforms`
list and verifies sibling specs declare a matching `platform_identifier` for
the same component. Mismatches are hard failures (`[semantic]` prefix);
missing sibling specs are warnings (`[warn]` prefix) so a not-yet-onboarded
sibling doesn't block this spec.

CLI:
    python3 -m fboss.platform.firmware_onboarding.validator.spec_validator <spec.json>
or via Buck:
    buck2 run //fboss/platform/firmware_onboarding/validator:validate -- <spec.json>
"""

from __future__ import annotations

import copy
import functools
import json
import re
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import cast

import jsonschema
from pydantic import BaseModel, ConfigDict, Field, model_validator, ValidationError


_PCIE_ADDRESS_RE: re.Pattern[str] = re.compile(r"^[0-9a-f]{2}:[0-9a-f]{2}\.[0-7]$")
_RANGE_RE: re.Pattern[str] = re.compile(r"^([1-9][0-9]*)-([1-9][0-9]*)$")


class _Instances(BaseModel):
    model_config = ConfigDict(extra="forbid")

    range: str
    position: str

    @model_validator(mode="after")
    def _check(self) -> _Instances:  # noqa: B902 — pydantic mode="after" instance method
        match = _RANGE_RE.match(self.range)
        if match is None:
            raise ValueError(
                f"instances.range '{self.range}' is malformed; "
                "expected '<start>-<end>' with positive integers"
            )
        start, end = int(match.group(1)), int(match.group(2))
        if end < start:
            raise ValueError(
                f"instances.range '{self.range}' has end ({end}) < start ({start})"
            )
        if self.position not in ("at_end", "before_first_underscore"):
            raise ValueError(
                f"instances.position '{self.position}' must be "
                "'at_end' or 'before_first_underscore'"
            )
        return self

    def expanded_indices(self) -> list[int]:
        match = _RANGE_RE.match(self.range)
        assert match is not None
        return list(range(int(match.group(1)), int(match.group(2)) + 1))


class _Firmware(BaseModel):
    model_config = ConfigDict(extra="forbid")

    priority: int = Field(ge=1, le=100)
    shared_with_platforms: list[str]


class _Component(BaseModel):
    model_config = ConfigDict(extra="forbid")

    name: str
    platform_identifier: str | None = None
    location: str
    display_location: str
    field_replaceable: bool
    instances: _Instances | None = None
    pcie_address: str | list[str] | None = None
    firmware: _Firmware

    @model_validator(mode="after")
    def _cross_field(self) -> _Component:  # noqa: B902 — pydantic mode="after" instance method
        # `before_first_underscore` requires an underscore in the base name.
        if (
            self.instances is not None
            and self.instances.position == "before_first_underscore"
            and "_" not in self.name
        ):
            raise ValueError(
                f"component '{self.name}': instances.position is "
                "'before_first_underscore' but the name has no underscore"
            )

        # `pcie_address` shape must match `instances` presence.
        if self.pcie_address is not None:
            if self.instances is None and not isinstance(self.pcie_address, str):
                raise ValueError(
                    f"component '{self.name}': pcie_address must be a string "
                    "when 'instances' is absent"
                )
            if self.instances is not None and not isinstance(self.pcie_address, list):
                raise ValueError(
                    f"component '{self.name}': pcie_address must be a list "
                    "when 'instances' is present"
                )
            if isinstance(self.pcie_address, list) and self.instances is not None:
                expected = len(self.instances.expanded_indices())
                got = len(self.pcie_address)
                if got != expected:
                    raise ValueError(
                        f"component '{self.name}': pcie_address has {got} "
                        f"entries but instances range '{self.instances.range}' "
                        f"covers {expected} instances"
                    )

            # Format check on every address (string or list elements).
            addresses = (
                [self.pcie_address]
                if isinstance(self.pcie_address, str)
                else self.pcie_address
            )
            for addr in addresses:
                if not _PCIE_ADDRESS_RE.match(addr):
                    raise ValueError(
                        f"component '{self.name}': pcie_address '{addr}' "
                        "is not in BB:DD.F format (lowercase hex)"
                    )

        return self

    def effective_instance_names(self) -> list[str]:
        if self.instances is None:
            return [self.name]
        names: list[str] = []
        for index in self.instances.expanded_indices():
            if self.instances.position == "at_end":
                names.append(f"{self.name}{index}")
            else:  # before_first_underscore
                head, _, tail = self.name.partition("_")
                names.append(f"{head}{index}_{tail}")
        return names


class _Silicon(BaseModel):
    model_config = ConfigDict(extra="forbid")

    name: str = Field(min_length=1)
    pcie_address: str

    @model_validator(mode="after")
    def _check(self) -> _Silicon:  # noqa: B902 — pydantic mode="after" instance method
        if not _PCIE_ADDRESS_RE.match(self.pcie_address):
            raise ValueError(
                f"silicon '{self.name}': pcie_address '{self.pcie_address}' "
                "is not in BB:DD.F format (lowercase hex)"
            )
        return self


class _Platform(BaseModel):
    model_config = ConfigDict(extra="forbid")

    name: str = Field(pattern=r"^[a-z][a-z0-9_]*$")
    silicon: list[_Silicon] = Field(min_length=1)


class _Spec(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)

    spec_version: str
    platform: _Platform
    components: list[_Component] = Field(min_length=1)
    review_notes: list[str] = Field(default_factory=list, alias="_review_notes")

    @model_validator(mode="after")
    def _post_expansion_uniqueness(self) -> _Spec:  # noqa: B902 — pydantic mode="after" instance method
        seen: dict[str, str] = {}
        collisions: list[str] = []
        for component in self.components:
            for name in component.effective_instance_names():
                if name in seen:
                    collisions.append(
                        f"'{name}' is produced by both '{seen[name]}' "
                        f"and '{component.name}'"
                    )
                else:
                    seen[name] = component.name
        if collisions:
            raise ValueError(
                "effective instance name collisions ("
                + str(len(collisions))
                + "):\n  - "
                + "\n  - ".join(collisions)
            )
        return self


@functools.lru_cache(maxsize=1)
def _load_json_schema_cached() -> dict[str, object]:
    """Internal cached loader — returns the canonical (shared, MUST NOT be
    mutated) schema dict. Public callers use `_load_json_schema()` which
    deep-copies the result so a mutating caller can't corrupt the cache."""
    # Buck-packaged: schema is bundled as a resource of the validator library.
    try:
        from importlib.resources import files

        package = __package__ or "fboss.platform.firmware_onboarding.validator"
        schema_text = (files(package) / "platform-spec.v1.json").read_text()
        return json.loads(schema_text)
    except (FileNotFoundError, ModuleNotFoundError):
        pass

    # Source tree: fall back to the on-disk schema next to the spec.
    schema_path = (
        Path(__file__).resolve().parent.parent / "schemas" / "platform-spec.v1.json"
    )
    return json.loads(schema_path.read_text())


def _load_json_schema() -> dict[str, object]:
    """Return a fresh (deep-copied) schema dict on every call. The underlying
    parse/load happens once via `_load_json_schema_cached`; the deep copy
    here ensures that a caller (or test) mutating the returned schema can't
    corrupt subsequent validations process-wide.
    """
    return copy.deepcopy(_load_json_schema_cached())


def validate_spec_dict(spec_obj: dict[str, object]) -> list[str]:
    """Validate a parsed spec dict; return error messages (empty list = valid)."""
    errors: list[str] = []

    try:
        jsonschema.validate(instance=spec_obj, schema=_load_json_schema())
    except jsonschema.ValidationError as e:
        path = ".".join(str(p) for p in e.absolute_path) or "<root>"
        errors.append(f"[schema] {path}: {e.message}")
        # If structural validation fails the shape can't be trusted; skip tier 2.
        return errors

    try:
        _Spec.model_validate(spec_obj)
    except ValidationError as e:
        for err in e.errors():
            loc = ".".join(str(x) for x in err["loc"]) or "<root>"
            errors.append(f"[semantic] {loc}: {err['msg']}")

    return errors


def validate_spec_file(
    spec_path: Path,
    configs_root: Path | None = None,
) -> list[str]:
    """Validate a spec file by path; return messages (`[schema]` / `[semantic]`
    are hard failures, `[warn]` is advisory).

    When `configs_root` is supplied, also runs the cross-spec
    `shared_with_platforms` check against sibling specs at
    `<configs_root>/<sibling_platform>/spec.json`. The check is skipped if
    tier-1 validation failed (the spec shape can't be trusted)."""
    try:
        spec_obj = json.loads(spec_path.read_text())
    except json.JSONDecodeError as e:
        return [
            f"[schema] {spec_path}: malformed JSON ({e.msg} at "
            f"line {e.lineno}, col {e.colno})"
        ]
    except OSError as e:
        return [f"[schema] {spec_path}: cannot read spec file ({e})"]
    messages = validate_spec_dict(spec_obj)

    if configs_root is not None and not any(m.startswith("[schema]") for m in messages):
        messages.extend(_check_shared_platforms(spec_obj, configs_root))

    return messages


def _load_sibling_spec(
    sibling: str,
    sibling_path: Path,
    comp_name: str,
) -> tuple[str | None, dict[str, object] | None, str | None]:
    """Load a sibling spec and locate the component matching `comp_name`.

    Returns `(sibling_platform_name, sibling_component, warn_message)`:
      - On success, the first two are populated and warn_message is None.
      - On failure (missing file, malformed JSON, unexpected structure,
        missing component), returns Nones for the data and a `[warn]` message
        the caller should append.
    """
    if not sibling_path.is_file():
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"references '{sibling}' but {sibling_path} does not exist "
                f"(may not be onboarded yet)"
            ),
        )
    try:
        sibling_obj_raw = json.loads(sibling_path.read_text())
    except (OSError, json.JSONDecodeError) as e:
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling spec at {sibling_path} could not be read ({e})"
            ),
        )

    # The sibling spec hasn't gone through tier 1+2 validation, so its
    # structure may not match the schema; surface unexpected structure as a
    # graceful [warn] instead of crashing the whole pre-flight.
    if not isinstance(sibling_obj_raw, dict):
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling spec at {sibling_path} has unexpected structure "
                f"(top-level is not an object); cannot verify shared firmware"
            ),
        )
    sibling_obj = cast(dict[str, object], sibling_obj_raw)
    sibling_platform_block = sibling_obj.get("platform")
    if not isinstance(sibling_platform_block, dict):
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling spec at {sibling_path} has unexpected structure "
                f"('platform' is not an object); cannot verify shared firmware"
            ),
        )
    # Sibling spec is unvalidated; `platform.name` may be missing, a list, an
    # int, etc. Force a graceful `[warn]` rather than letting a non-string slip
    # into `_compare_identifiers`, which would otherwise emit a confusing
    # `[semantic]` mismatch embedding the malformed value. Mirrors the same
    # isinstance(str) guard that `_compare_identifiers` applies to
    # `platform_identifier` on the per-component side.
    raw_name = sibling_platform_block.get("name", sibling)
    if not isinstance(raw_name, str):
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling spec at {sibling_path} has unexpected structure "
                f"('platform.name' is not a string); cannot verify shared firmware"
            ),
        )
    sibling_platform = raw_name

    sibling_components = sibling_obj.get("components", [])
    if not isinstance(sibling_components, list):
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling spec at {sibling_path} has unexpected structure "
                f"('components' is not a list); cannot verify shared firmware"
            ),
        )

    sibling_comp = next(
        (
            c
            for c in sibling_components
            if isinstance(c, dict) and c.get("name") == comp_name
        ),
        None,
    )
    if sibling_comp is None:
        return (
            None,
            None,
            (
                f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
                f"sibling platform '{sibling}' has no component named "
                f"'{comp_name}' — cannot verify shared firmware"
            ),
        )

    return sibling_platform, sibling_comp, None


def _compare_identifiers(
    comp_name: str,
    own_identifier: str,
    sibling: str,
    sibling_platform: str,
    sibling_comp: dict[str, object],
) -> str | None:
    """Compare `platform_identifier` between own and sibling component.

    Returns a `[semantic]` hard-failure message if they diverge, a `[warn]`
    if the sibling's `platform_identifier` is present but malformed (e.g. a
    list or number), or None if they agree. Mirrors own_identifier's
    fallback: uses the sibling spec's declared `platform.name` when
    `platform_identifier` is omitted, so a sibling whose directory name
    diverges from its declared platform.name still compares correctly.
    """
    raw_sibling_id = sibling_comp.get("platform_identifier")
    if raw_sibling_id is not None and not isinstance(raw_sibling_id, str):
        # Sibling spec wasn't run through tier 1+2, so its
        # `platform_identifier` may be malformed. Surface gracefully as a
        # [warn] instead of producing a confusing [semantic] message with
        # cast-mangled output.
        return (
            f"[warn] components.{comp_name}.firmware.shared_with_platforms: "
            f"sibling platform '{sibling}' declares a non-string "
            f"platform_identifier ({type(raw_sibling_id).__name__}); "
            f"cannot verify shared firmware"
        )
    sibling_identifier = raw_sibling_id or sibling_platform
    if own_identifier == sibling_identifier:
        return None
    return (
        f"[semantic] components.{comp_name}: declares "
        f"platform_identifier='{own_identifier}' but sibling "
        f"platform '{sibling}' declares "
        f"platform_identifier='{sibling_identifier}' for the same "
        f"component. Shared firmware images MUST use the same "
        f"platform_identifier across all platforms that share them."
    )


def _check_shared_platforms(
    spec_obj: dict[str, object], configs_root: Path
) -> list[str]:
    """Cross-spec check for `firmware.shared_with_platforms`.

    For each component, look up every platform listed in
    `firmware.shared_with_platforms`:
      - Sibling spec missing → `[warn]` (not yet onboarded).
      - Sibling spec exists but has no component with this name → `[warn]`
        (firmware sharing claim is unverifiable).
      - Sibling spec exists and declares a different `platform_identifier`
        for the same component → `[semantic]` hard failure (shared firmware
        images MUST use a single `platform_identifier` across every platform
        that shares them, otherwise Meta would create duplicate firmware
        records).
    """
    issues: list[str] = []
    # Tier 1+2 has already validated the shape, so these casts are safe.
    own_platform = cast(str, cast(dict[str, object], spec_obj["platform"])["name"])
    components = cast(list[dict[str, object]], spec_obj.get("components", []))

    for comp in components:
        # Tier 1+2 has already validated the shape: `name` is a required
        # string, `firmware` is a required object, `shared_with_platforms`
        # defaults to []. Index/get directly rather than defensive `or {}`
        # / `or []` fallbacks that obscured those invariants.
        comp_name = cast(str, comp["name"])
        own_identifier = cast(str, comp.get("platform_identifier") or own_platform)
        firmware_block = cast(dict[str, object], comp["firmware"])
        siblings = cast(list[str], firmware_block.get("shared_with_platforms", []))

        for sibling in siblings:
            sibling_path = configs_root / sibling / "spec.json"
            sibling_platform, sibling_comp, warn = _load_sibling_spec(
                sibling, sibling_path, comp_name
            )
            if warn is not None:
                issues.append(warn)
                continue
            assert sibling_platform is not None and sibling_comp is not None
            mismatch = _compare_identifiers(
                comp_name, own_identifier, sibling, sibling_platform, sibling_comp
            )
            if mismatch is not None:
                issues.append(mismatch)

    return issues


def main(argv: Sequence[str] | None = None) -> int:
    args = list(sys.argv if argv is None else argv)
    progname = args[0] if args else "spec_validator"
    if len(args) != 2:
        print(f"usage: {progname} <spec.json>", file=sys.stderr)
        return 2
    spec_path = Path(args[1]).resolve()
    # Best-effort configs_root: the spec lives at
    # `<configs_root>/<platform>/spec.json` in the canonical layout. We only
    # enable the cross-spec check when the layout actually looks canonical —
    # checked by requiring at least one other `<sibling>/spec.json` to exist
    # alongside this one. A bare relative path like `spec.json` (resolved to
    # `<cwd>/spec.json`) would otherwise enable the check against arbitrary
    # subdirectories of the cwd's parent, which is almost never intentional.
    configs_root: Path | None = None
    grandparent = spec_path.parent.parent
    if grandparent != spec_path.parent and grandparent.is_dir():
        own_dirname = spec_path.parent.name
        # Look for any sibling `<dir>/spec.json` (other than ourselves) to
        # confirm grandparent is really a configs_root.
        if any(
            (child / "spec.json").is_file()
            for child in grandparent.iterdir()
            if child.is_dir() and child.name != own_dirname
        ):
            configs_root = grandparent
    messages = validate_spec_file(spec_path, configs_root=configs_root)
    if not any(m.startswith(("[schema]", "[semantic]")) for m in messages):
        # Print warnings (if any) to stderr but still exit 0.
        for msg in messages:
            print(msg, file=sys.stderr)
        print("OK")
        return 0
    for msg in messages:
        print(msg, file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
