// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/hw/bcm/BcmLabelSwitchingUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

// need to define bde in a cpp_unittest for linking with sdk library
extern "C" {
struct ibde_t;
ibde_t* bde;
}

using namespace ::testing;
namespace facebook::fboss {

using namespace facebook::fboss::utility;

TEST(BcmLabelSwitchingUtilsTests, getLabelSwitchAction) {
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_PHP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::DROP,
          LabelForwardingAction::LabelForwardingType::PUSH));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_PHP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::TO_CPU,
          LabelForwardingAction::LabelForwardingType::PUSH));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_SWAP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::NEXTHOPS,
          LabelForwardingAction::LabelForwardingType::PUSH));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_PHP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::NEXTHOPS,
          LabelForwardingAction::LabelForwardingType::PHP));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_SWAP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::NEXTHOPS,
          LabelForwardingAction::LabelForwardingType::SWAP));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_POP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::NEXTHOPS,
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP));
  EXPECT_EQ(
      BCM_MPLS_SWITCH_ACTION_NOP,
      getLabelSwitchAction(
          LabelNextHopEntry::Action::NEXTHOPS,
          LabelForwardingAction::LabelForwardingType::NOOP));
}

TEST(BcmLabelSwitchingUtilsTests, isValidLabeledNextHop) {
  EXPECT_EQ(
      isValidLabeledNextHop(
          9,
          UnresolvedNextHop(
              folly::IPAddressV4("10.0.0.1"),
              NextHopWeight(0),
              LabelForwardingAction(
                  LabelForwardingAction::LabelForwardingType::PHP))),
      false);
  EXPECT_EQ(
      isValidLabeledNextHop(
          9,
          UnresolvedNextHop(
              folly::IPAddressV4("10.0.0.1"),
              NextHopWeight(0),
              LabelForwardingAction(
                  LabelForwardingAction::LabelForwardingType::PUSH,
                  {1, 2, 3, 4, 5, 6, 7, 8, 9}))),
      false);

  EXPECT_EQ(
      isValidLabeledNextHop(
          9,
          ResolvedNextHop(
              folly::IPAddressV4("10.0.0.1"),
              InterfaceID(1),
              NextHopWeight(0),
              LabelForwardingAction(
                  LabelForwardingAction::LabelForwardingType::PUSH,
                  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}))),
      false);

  EXPECT_EQ(
      isValidLabeledNextHop(
          9,
          ResolvedNextHop(
              folly::IPAddressV4("10.0.0.1"),
              InterfaceID(1),
              NextHopWeight(0),
              LabelForwardingAction(
                  LabelForwardingAction::LabelForwardingType::PUSH,
                  {1, 2, 3, 4, 5, 6, 7, 8, 9}))),
      true);
}

TEST(BcmLabelSwitchingUtilsTests, isValidLabeledNextHopSet) {
  LabelNextHopSet validNextHops{
      ResolvedNextHop(
          folly::IPAddressV4("10.0.0.1"),
          InterfaceID(1),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PHP)),
      ResolvedNextHop(
          folly::IPAddressV4("10.0.0.2"),
          InterfaceID(2),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PHP)),
      ResolvedNextHop(
          folly::IPAddressV4("10.0.0.3"),
          InterfaceID(3),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PHP)),
      ResolvedNextHop(
          folly::IPAddressV4("10.0.0.4"),
          InterfaceID(4),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PHP)),

  };
  LabelNextHopSet pushNextHopSet = {
      ResolvedNextHop(
          folly::IPAddressV4("10.0.0.5"),
          InterfaceID(5),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PUSH,
              {1, 2, 3, 4, 5, 6, 7, 8, 9})),
  };
  LabelNextHopSet unresolvedNextHopSet = {
      UnresolvedNextHop(
          folly::IPAddressV4("10.0.0.5"),
          NextHopWeight(0),
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PUSH,
              {1, 2, 3, 4, 5, 6, 7, 8, 9})),
  };
  EXPECT_EQ(true, isValidLabeledNextHopSet(9, validNextHops));
  EXPECT_EQ(false, isValidLabeledNextHopSet(8, pushNextHopSet));
  EXPECT_EQ(true, isValidLabeledNextHopSet(9, pushNextHopSet));
  validNextHops.merge(std::move(pushNextHopSet));
  EXPECT_EQ(false, isValidLabeledNextHopSet(9, validNextHops));
  validNextHops.erase(ResolvedNextHop(
      folly::IPAddressV4("10.0.0.5"),
      InterfaceID(5),
      NextHopWeight(0),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          {1, 2, 3, 4, 5, 6, 7, 8, 9})));
  validNextHops.merge(std::move(unresolvedNextHopSet));
  EXPECT_EQ(false, isValidLabeledNextHopSet(9, validNextHops));
}

} // namespace facebook::fboss
