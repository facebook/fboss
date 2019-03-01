/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

using namespace facebook::fboss;

class NextHopManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
  }

  void checkNextHop(
      sai_object_id_t nextHopId,
      sai_object_id_t expectedRifId,
      const folly::IPAddress& expectedIp) {
    auto rifIdGot = saiApiTable->nextHopApi().getAttribute(
        NextHopApiParameters::Attributes::RouterInterfaceId(), nextHopId);
    EXPECT_EQ(rifIdGot, expectedRifId);
    auto ipGot = saiApiTable->nextHopApi().getAttribute(
        NextHopApiParameters::Attributes::Ip(), nextHopId);
    EXPECT_EQ(ipGot, expectedIp);
    auto typeGot = saiApiTable->nextHopApi().getAttribute(
        NextHopApiParameters::Attributes::Type(), nextHopId);
    EXPECT_EQ(typeGot, SAI_NEXT_HOP_TYPE_IP);
  }
};

TEST_F(NextHopManagerTest, testAddNextHop) {
  sai_object_id_t rifId{42};
  folly::IPAddress ip4{"42.42.42.42"};
  std::unique_ptr<SaiNextHop> nextHop =
      saiManagerTable->nextHopManager().addNextHop(rifId, ip4);
  checkNextHop(nextHop->id(), rifId, ip4);
}
