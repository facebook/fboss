// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingAction.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

namespace {
void testFromAndTo(const LabelForwardingAction& action) {
  EXPECT_EQ(
      action, LabelForwardingAction::fromFollyDynamic(action.toFollyDynamic()));
  EXPECT_EQ(action, LabelForwardingAction::fromThrift(action.toThrift()));
}
} // namespace

TEST(LabelForwardingActionTests, SwapThriftDynamic) {
  LabelForwardingAction action(
      LabelForwardingAction::LabelForwardingType::SWAP, 1001);
  testFromAndTo(action);
  EXPECT_EQ(action.str(), "MPLS: SWAP -> 1001");
}

TEST(LabelForwardingActionTests, PushThriftDynamic) {
  LabelForwardingAction::LabelStack stack{1001, 1002, 1003};
  LabelForwardingAction action(
      LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack));
  testFromAndTo(action);
  EXPECT_EQ(action.str(), "MPLS: PUSH -> [ 1001, 1002, 1003 ]");
}

TEST(LabelForwardingActionTests, PopThriftDynamic) {
  LabelForwardingAction action(
      LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  testFromAndTo(action);
  EXPECT_EQ(action.str(), "MPLS: POP_AND_LOOKUP");
}

TEST(LabelForwardingActionTests, PhpThriftDynamic) {
  LabelForwardingAction action(LabelForwardingAction::LabelForwardingType::PHP);
  testFromAndTo(action);
  EXPECT_EQ(action.str(), "MPLS: PHP");
}

TEST(LabelForwardingActionTests, NoopThriftDynamic) {
  LabelForwardingAction action(
      LabelForwardingAction::LabelForwardingType::NOOP);
  testFromAndTo(action);
  EXPECT_EQ(action.str(), "MPLS: NOOP");
}

TEST(LabelForwardingActionTests, InvalidLabelForwardingAction) {
  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::SWAP);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::SWAP,
            {1001, 1002, 1003});
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::PUSH);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::PUSH, 1001);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::PHP,
            {1001, 1002, 1003});
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::PHP, 1001);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP,
            {1001, 1002, 1003});
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP, 1001);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::NOOP, 1001);
      },
      FbossError);

  EXPECT_THROW(
      {
        LabelForwardingAction action(
            LabelForwardingAction::LabelForwardingType::NOOP,
            {1001, 1002, 1003});
      },
      FbossError);
}
