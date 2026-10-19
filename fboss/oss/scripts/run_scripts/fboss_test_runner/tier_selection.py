#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import enum
from collections.abc import Collection, Iterable
from dataclasses import dataclass

__test__ = False


class TestTier(str, enum.Enum):
    UNSPECIFIED = "UNSPECIFIED"
    T0 = "T0"
    T1 = "T1"
    T2 = "T2"


class TestCapability(str, enum.Enum):
    TRANSCEIVER = "TRANSCEIVER"
    EXTERNAL_PHY = "EXTERNAL_PHY"


class TestProfile(str, enum.Enum):
    TRADITIONAL = "TRADITIONAL"
    SCALE_UP = "SCALE_UP"


@dataclass(frozen=True)
class TestMetadataRule:
    test_name_pattern: str
    tier: TestTier
    required_capabilities: tuple[TestCapability, ...] = ()
    profiles: tuple[TestProfile, ...] = ()


_CLI_TIERS = {
    "0": TestTier.T0,
    "1": TestTier.T1,
    "2": TestTier.T2,
}


def parse_test_tier(value: str) -> TestTier:
    """Parse the numeric spelling used by ``run_test.py --tier``."""
    try:
        return _CLI_TIERS[value]
    except KeyError as error:
        raise ValueError(f"invalid test tier: {value!r}") from error


def _profile_matches(
    rule_profiles: Collection[TestProfile], requested_profile: TestProfile | None
) -> bool:
    if requested_profile is not None:
        return requested_profile in rule_profiles

    # Preserve the existing filter-file behavior: without --profile, select
    # untagged rules and rules tagged for the traditional profile.
    return not rule_profiles or TestProfile.TRADITIONAL in rule_profiles


def select_test_patterns_for_tier(
    rules: Iterable[TestMetadataRule],
    tier: TestTier,
    *,
    available_capabilities: Collection[TestCapability] = (),
    profile: TestProfile | None = None,
) -> list[str]:
    """Return GTest filter patterns from the applicable rules for ``tier``.

    A rule is applicable when its tier and profile match and all its required
    capabilities are available. A pattern may appear in multiple tiers and is
    therefore selectable in each of those tier runs. Rule order is preserved
    exactly. The caller passes these patterns to GTest, which remains
    authoritative for pattern expansion.
    """
    return [
        rule.test_name_pattern
        for rule in rules
        if (
            rule.tier is tier
            and set(rule.required_capabilities).issubset(available_capabilities)
            and _profile_matches(rule.profiles, profile)
        )
    ]
