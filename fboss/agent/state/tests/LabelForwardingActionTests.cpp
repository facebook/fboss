// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "folly/IPAddress.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;
using facebook::network::toBinaryAddress;

namespace {
void testFromAndTo(const LabelForwardingAction& action) {
  EXPECT_EQ(action, LabelForwardingAction::fromThrift(action.toThrift()));
}

NextHopThrift getMplsNextHop(
    const folly::IPAddress& ip,
    InterfaceID interface,
    const LabelForwardingAction& action) {
  NextHopThrift nhop;
  nhop.address() = toBinaryAddress(ip);
  nhop.address()->ifName() = folly::to<std::string>("fboss", interface);
  nhop.mplsAction() = action.toThrift();
  return nhop;
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

TEST(LabelForwardingActionTests, MplsNextHopThrift) {
  std::vector<LabelForwardingAction> actions{
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, {1001, 1002}),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP),
  };

  for (auto ip : {"1.0.0.1", "1::1", "fe80::1"}) {
    for (auto& action : actions) {
      auto nhopThrift =
          getMplsNextHop(folly::IPAddress(ip), InterfaceID(10), action);
      auto nhop = util::fromThrift(nhopThrift);
      EXPECT_EQ(nhop.labelForwardingAction(), action);
      if (action.type() !=
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
        EXPECT_EQ(nhop.addr(), folly::IPAddress(ip));
      }
      if (folly::IPAddress(ip).isV6() && folly::IPAddress(ip).isLinkLocal()) {
        EXPECT_EQ(nhop.intf(), InterfaceID(10));
      }
    }
  }
}

TEST(LabelForwardingActionTests, MplsNextHopThriftSet) {
  std::vector<LabelForwardingAction> actions{
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, {1001, 1002}),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP),
  };
  for (auto& action : actions) {
    std::vector<NextHopThrift> nhops_vec;
    auto i = 1;

    for (auto ip : {"fe80::1", "fe80::2"}) {
      nhops_vec.push_back(
          getMplsNextHop(folly::IPAddress(ip), InterfaceID(i++), action));
      if (action.type() ==
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
        // only one next hop for pop and look up action
        break;
      }
    }
    auto nhops = util::toRouteNextHopSet(nhops_vec);
    EXPECT_TRUE(MultiLabelForwardingInformationBase::isValidNextHopSet(nhops));
  }
}
