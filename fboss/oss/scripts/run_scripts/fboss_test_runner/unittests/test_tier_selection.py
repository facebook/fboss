#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import pytest
from fboss_test_runner.tier_selection import (
    parse_test_tier,
    select_test_patterns_for_tier,
    TestCapability,
    TestMetadataRule,
    TestProfile,
    TestTier,
)


def _rule(
    pattern: str,
    tier: TestTier,
    *,
    capabilities: frozenset[TestCapability] = frozenset(),
    profiles: frozenset[TestProfile] = frozenset(),
) -> TestMetadataRule:
    return TestMetadataRule(
        test_name_pattern=pattern,
        tier=tier,
        required_capabilities=tuple(capabilities),
        profiles=tuple(profiles),
    )


def test_test_tier_parses_cli_values() -> None:
    assert parse_test_tier("0") is TestTier.T0
    assert parse_test_tier("1") is TestTier.T1
    assert parse_test_tier("2") is TestTier.T2


def test_test_tier_rejects_invalid_cli_value() -> None:
    with pytest.raises(ValueError, match="invalid test tier"):
        parse_test_tier("3")


def test_selects_only_requested_tier_and_preserves_rule_order() -> None:
    rules = [
        _rule("*Route*", TestTier.T0),
        _rule("EmptyTest.*", TestTier.T1),
        _rule("*Vlan*", TestTier.T0),
    ]

    assert select_test_patterns_for_tier(rules, TestTier.T0) == [
        "*Route*",
        "*Vlan*",
    ]


def test_requires_every_capability() -> None:
    rules = [
        _rule(
            "HwTest.*",
            TestTier.T0,
            capabilities=frozenset(
                {TestCapability.TRANSCEIVER, TestCapability.EXTERNAL_PHY}
            ),
        )
    ]

    assert (
        select_test_patterns_for_tier(
            rules,
            TestTier.T0,
            available_capabilities={TestCapability.TRANSCEIVER},
        )
        == []
    )
    assert select_test_patterns_for_tier(
        rules,
        TestTier.T0,
        available_capabilities={
            TestCapability.TRANSCEIVER,
            TestCapability.EXTERNAL_PHY,
        },
    ) == ["HwTest.*"]


def test_default_profile_matches_untagged_and_traditional_rules() -> None:
    rules = [
        _rule("Suite.Untagged", TestTier.T0),
        _rule(
            "Suite.Traditional",
            TestTier.T0,
            profiles=frozenset({TestProfile.TRADITIONAL}),
        ),
        _rule(
            "Suite.ScaleUp",
            TestTier.T0,
            profiles=frozenset({TestProfile.SCALE_UP}),
        ),
    ]

    assert select_test_patterns_for_tier(
        rules,
        TestTier.T0,
    ) == ["Suite.Untagged", "Suite.Traditional"]


def test_explicit_profile_matches_only_rules_with_that_profile() -> None:
    rules = [
        _rule("Suite.Untagged", TestTier.T0),
        _rule(
            "Suite.Both",
            TestTier.T0,
            profiles=frozenset({TestProfile.TRADITIONAL, TestProfile.SCALE_UP}),
        ),
        _rule(
            "Suite.ScaleUp",
            TestTier.T0,
            profiles=frozenset({TestProfile.SCALE_UP}),
        ),
    ]

    assert select_test_patterns_for_tier(
        rules,
        TestTier.T0,
        profile=TestProfile.SCALE_UP,
    ) == ["Suite.Both", "Suite.ScaleUp"]


def test_pattern_can_be_selected_in_multiple_tiers() -> None:
    rules = [
        _rule("Suite.Shared", TestTier.T0),
        _rule("Suite.Shared", TestTier.T1),
    ]

    assert select_test_patterns_for_tier(rules, TestTier.T0) == ["Suite.Shared"]
    assert select_test_patterns_for_tier(rules, TestTier.T1) == ["Suite.Shared"]
