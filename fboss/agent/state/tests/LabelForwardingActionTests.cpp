// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "folly/IPAddress.h"

#include <folly/json/dynamic.h>
#include <gtest/gtest.h>
#include <unordered_set>

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

TEST(LabelForwardingActionTests, HashFunction) {
  // Test that equal objects have the same hash
  LabelForwardingAction action1(
      LabelForwardingAction::LabelForwardingType::SWAP, 1001);
  LabelForwardingAction action2(
      LabelForwardingAction::LabelForwardingType::SWAP, 1001);

  std::hash<LabelForwardingAction> hasher;
  EXPECT_EQ(hasher(action1), hasher(action2));

  // Test that different objects typically have different hashes
  LabelForwardingAction action3(
      LabelForwardingAction::LabelForwardingType::SWAP, 1002);
  EXPECT_NE(hasher(action1), hasher(action3));
}

TEST(LabelForwardingActionTests, HashFunctionPushActions) {
  std::hash<LabelForwardingAction> hasher;

  // Test equal PUSH actions have same hash
  LabelForwardingAction::LabelStack stack1{1001, 1002, 1003};
  LabelForwardingAction action1(
      LabelForwardingAction::LabelForwardingType::PUSH, stack1);

  LabelForwardingAction::LabelStack stack2{1001, 1002, 1003};
  LabelForwardingAction action2(
      LabelForwardingAction::LabelForwardingType::PUSH, stack2);

  EXPECT_EQ(hasher(action1), hasher(action2));

  // Different stack should have different hash
  LabelForwardingAction::LabelStack stack3{1001, 1002, 1004};
  LabelForwardingAction action3(
      LabelForwardingAction::LabelForwardingType::PUSH, stack3);

  EXPECT_NE(hasher(action1), hasher(action3));

  // Different stack order should have different hash
  LabelForwardingAction::LabelStack stack4{1003, 1002, 1001};
  LabelForwardingAction action4(
      LabelForwardingAction::LabelForwardingType::PUSH, stack4);

  EXPECT_NE(hasher(action1), hasher(action4));
}

TEST(LabelForwardingActionTests, HashFunctionSimpleActions) {
  std::hash<LabelForwardingAction> hasher;

  // Test simple actions (PHP, POP_AND_LOOKUP, NOOP)
  LabelForwardingAction php1(LabelForwardingAction::LabelForwardingType::PHP);
  LabelForwardingAction php2(LabelForwardingAction::LabelForwardingType::PHP);
  LabelForwardingAction pop(
      LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  LabelForwardingAction noop(LabelForwardingAction::LabelForwardingType::NOOP);

  // Same actions should have same hash
  EXPECT_EQ(hasher(php1), hasher(php2));

  // Different actions should have different hashes
  EXPECT_NE(hasher(php1), hasher(pop));
  EXPECT_NE(hasher(php1), hasher(noop));
  EXPECT_NE(hasher(pop), hasher(noop));
}

TEST(LabelForwardingActionTests, HashFunctionUnorderedSet) {
  // Test that hash function works with unordered containers
  std::unordered_set<LabelForwardingAction> actionSet;

  // Add different types of actions
  actionSet.emplace(LabelForwardingAction::LabelForwardingType::SWAP, 1001);
  actionSet.emplace(
      LabelForwardingAction::LabelForwardingType::PUSH,
      LabelForwardingAction::LabelStack{1001, 1002});
  actionSet.emplace(LabelForwardingAction::LabelForwardingType::PHP);
  actionSet.emplace(LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  actionSet.emplace(LabelForwardingAction::LabelForwardingType::NOOP);

  EXPECT_EQ(actionSet.size(), 5);

  // Try to add duplicates - set size should remain the same
  actionSet.emplace(LabelForwardingAction::LabelForwardingType::SWAP, 1001);
  actionSet.emplace(
      LabelForwardingAction::LabelForwardingType::PUSH,
      LabelForwardingAction::LabelStack{1001, 1002});

  EXPECT_EQ(actionSet.size(), 5);

  // Test that we can find items
  LabelForwardingAction swapAction(
      LabelForwardingAction::LabelForwardingType::SWAP, 1001);
  EXPECT_NE(actionSet.find(swapAction), actionSet.end());

  LabelForwardingAction notFoundAction(
      LabelForwardingAction::LabelForwardingType::SWAP, 9999);
  EXPECT_EQ(actionSet.find(notFoundAction), actionSet.end());
}

TEST(LabelForwardingActionTests, HashConsistency) {
  // Test that hash is consistent across multiple calls
  LabelForwardingAction action(
      LabelForwardingAction::LabelForwardingType::PUSH,
      LabelForwardingAction::LabelStack{1001, 1002, 1003});

  std::hash<LabelForwardingAction> hasher;
  size_t hash1 = hasher(action);
  size_t hash2 = hasher(action);
  size_t hash3 = hasher(action);

  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
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
