/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/ResourceAccountant.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

class ResourceAccountantTest : public ::testing::Test {
 public:
  void SetUp() override {
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo{
        {0, createSwitchInfo(cfg::SwitchType::NPU)}};
    asicTable_ =
        std::make_unique<HwAsicTable>(switchIdToSwitchInfo, std::nullopt);
    resourceAccountant_ =
        std::make_unique<ResourceAccountant>(asicTable_.get());
  }

  std::unique_ptr<ResourceAccountant> resourceAccountant_;
  std::unique_ptr<HwAsicTable> asicTable_;
};

TEST_F(ResourceAccountantTest, getMemberCountForEcmpGroup) {
  constexpr auto ecmpWeight = 1;
  constexpr auto ecmpWidth = 36;

  RouteNextHopSet ecmpNexthops;
  for (int i = 0; i < ecmpWidth; i++) {
    ecmpNexthops.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight));
  }
  RouteNextHopEntry ecmpNextHopEntry =
      RouteNextHopEntry(ecmpNexthops, AdminDistance::EBGP);
  EXPECT_EQ(
      ecmpWidth,
      this->resourceAccountant_->getMemberCountForEcmpGroup(ecmpNextHopEntry));

  RouteNextHopSet ucmpNexthops;
  for (int i = 0; i < ecmpWidth; i++) {
    ucmpNexthops.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        i + 1));
  }
  RouteNextHopEntry ucmpNextHopEntry =
      RouteNextHopEntry(ucmpNexthops, AdminDistance::EBGP);
  auto expectedTotalMember = 0;
  for (const auto& nhop : ucmpNextHopEntry.normalizedNextHops()) {
    expectedTotalMember += nhop.weight();
  }
  EXPECT_EQ(
      expectedTotalMember,
      this->resourceAccountant_->getMemberCountForEcmpGroup(ucmpNextHopEntry));
}

} // namespace facebook::fboss
