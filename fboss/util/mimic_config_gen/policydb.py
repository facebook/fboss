# Copyright (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

"""Find the coop inputs a {role, hardware} pair is missing, and source them.

PolicyDB indexes every forwarding-stack input by (role, hw) -- the artifacts
live at `inputs/<subresource>/<role>/<hw>/artifact` and each carries a netwhoami
scope. A pair that has never existed matches nothing, so coop cannot even
determine a path for it. We find those gaps and pick a donor artifact per gap.
"""

from __future__ import annotations

import logging
import typing as t

from fboss.util.mimic_config_gen.defs import (
    Blocker,
    cfgr_path,
    CONFLICT,
    Donors,
    EITHER,
    FORWARDING_STACK_RESOURCE,
    HW_SHAPED,
    MimicError,
    POLICYDB_RESOURCES_PATH,
    ROLE_SHAPED,
    Shape,
)

logger: logging.Logger = logging.getLogger(__name__)

# Inputs that must come from the target hardware whatever the 2x2 measurement
# says. Their content varies along both axes, so the pivot experiment reports
# CONFLICT; but the agent interprets them against the ASIC, so the hardware
# donor is always the right answer and the measurement is overridden.
_ALWAYS_HW_SHAPED = frozenset({"platform_mapping", "chip_config"})


class Scanner:
    """Resolves coop inputs for each corner of the donor 2x2."""

    def __init__(
        self,
        donors: Donors,
        base_whoami_json: t.Mapping[str, t.Any],
        hw_profiles: t.Mapping[str, tuple[str | None, str | None]],
    ) -> None:
        from configerator.client import ConfigeratorClient

        self._cc = ConfigeratorClient()
        self._content: dict[str, str] = {}

        # Every corner reuses the real source device's netwhoami and varies
        # only role/hw. Holding region, dc, tags and the rest constant is what
        # makes the corner-to-corner comparison a controlled experiment.
        from fboss.util.mimic_config_gen.forge import respin_json, to_thrift

        def corner(role: str, hw: str) -> t.Any:
            asic, vendor = hw_profiles.get(hw, (None, None))
            return to_thrift(respin_json(base_whoami_json, role, hw, asic, vendor))

        self._corners: dict[str, t.Any] = {
            "target": corner(donors.role, donors.target_hw),
            "role_donor": corner(donors.role, donors.source_hw),
            "hw_donor": corner(donors.bridge_role, donors.target_hw),
            "pivot": corner(donors.bridge_role, donors.source_hw),
        }

    def _subresources(self) -> t.Sequence[t.Any]:
        from facebook.neteng.policydb.resources.thrift_types import Resources

        resources = self._cc.get_config_contents_as_thrift(
            POLICYDB_RESOURCES_PATH, Resources
        )
        resource = resources.resources.get(FORWARDING_STACK_RESOURCE)
        if resource is None:
            raise MimicError(
                f"{FORWARDING_STACK_RESOURCE} missing from {POLICYDB_RESOURCES_PATH}"
            )
        return resource.subresources

    def _match(self, subresource: t.Any, corner: str) -> str | None:
        """Path the given corner resolves to, using coop's own scope matcher.

        Filters on artifact type exactly as coop does, so an fbpkg artifact is
        never mistaken for a configerator path.
        """
        from facebook.neteng.policydb.resources.thrift_types import ArtifactType
        from neteng.netwhoami.matcher import does_scope_match_whoami

        whoami = self._corners[corner]
        for artifact in subresource.artifacts:
            if artifact.artifact.atype != ArtifactType.CONFIGERATOR:
                continue
            if does_scope_match_whoami(artifact.scope, whoami):
                return cfgr_path(artifact.artifact.value)
        return None

    def _matches_ignoring_type(self, subresource: t.Any, corner: str) -> bool:
        """Whether the corner matches any artifact, configerator or not."""
        from neteng.netwhoami.matcher import does_scope_match_whoami

        whoami = self._corners[corner]
        return any(
            does_scope_match_whoami(a.scope, whoami) for a in subresource.artifacts
        )

    def fetch(self, path: str) -> str:
        if path not in self._content:
            raw = self._cc.get_config_contents(path)
            self._content[path] = raw.decode() if isinstance(raw, bytes) else raw
        return self._content[path]

    def _classify(
        self, name: str, role_path: str | None, hw_path: str | None, pivot: str | None
    ) -> Shape:
        """Decide whether an input varies with hardware, role, both or neither.

        Uses the pivot cohort {bridge_role, source_hw} to hold one axis fixed
        while varying the other, so the answer comes from PolicyDB's own data
        rather than a curated list that would rot as inputs are added.
        """
        if name in _ALWAYS_HW_SHAPED:
            if hw_path is None:
                # Falling back to the role donor here would hand coop the
                # SOURCE platform's mapping, which is precisely what this set
                # exists to prevent. Refuse rather than emit a wrong config.
                raise MimicError(
                    f"{name} must come from the target hardware, but the hw "
                    f"donor resolves nothing for it. The bridge role may have "
                    "no PolicyDB artifacts; try a different --bridge-role."
                )
            return HW_SHAPED
        if hw_path is None:
            return ROLE_SHAPED
        if role_path is None:
            return HW_SHAPED
        if pivot is None:
            # No pivot cohort, so the controlled experiment cannot be run.
            # Report it as a conflict rather than a confident verdict.
            return CONFLICT

        try:
            pivot_content = self.fetch(pivot)
            hw_varies = self.fetch(hw_path) != pivot_content
            role_varies = self.fetch(role_path) != pivot_content
        except Exception as e:  # noqa: BLE001 - any fetch failure is unclassifiable
            # Do not present a guess as a measurement: flag it for review.
            logger.warning("could not classify %s (%s)", name, e)
            return CONFLICT

        if hw_varies and role_varies:
            return CONFLICT
        if hw_varies:
            return HW_SHAPED
        if role_varies:
            return ROLE_SHAPED
        return EITHER

    def scan(self) -> tuple[list[Blocker], list[str]]:
        """Return (blockers with a chosen donor, blockers with no donor)."""
        blockers: list[Blocker] = []
        unsourceable: list[str] = []

        for sub in self._subresources():
            if self._match(sub, "target") is not None:
                continue  # target already resolves; nothing to do
            role_path = self._match(sub, "role_donor")
            hw_path = self._match(sub, "hw_donor")
            if role_path is None and hw_path is None:
                if self._matches_ignoring_type(sub, "role_donor"):
                    # The donor resolves this, but via a non-configerator
                    # artifact (an fbpkg name). Not something we can override,
                    # and worth saying so rather than dropping silently.
                    unsourceable.append(sub.name)
                # Otherwise genuinely out of scope for this device -- not a gap.
                continue

            shape = self._classify(
                sub.name, role_path, hw_path, self._match(sub, "pivot")
            )
            chosen_from = "hw_donor" if shape == HW_SHAPED else "role_donor"
            chosen = hw_path if chosen_from == "hw_donor" else role_path
            if chosen is None:
                chosen, chosen_from = (
                    (role_path, "role_donor")
                    if role_path is not None
                    else (hw_path, "hw_donor")
                )
            if chosen is None:
                unsourceable.append(sub.name)
                continue

            blockers.append(
                Blocker(
                    name=sub.name,
                    shape=shape,
                    chosen_path=chosen,
                    chosen_from=chosen_from,
                    role_path=role_path,
                    hw_path=hw_path,
                )
            )

        blockers.sort(key=lambda b: b.name)
        return blockers, sorted(unsourceable)
