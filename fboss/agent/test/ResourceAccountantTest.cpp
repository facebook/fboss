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

namespace {
constexpr auto ecmpWeight = 1;
}

namespace facebook::fboss {

class ResourceAccountantTest : public ::testing::Test {
 public:
  void SetUp() override {
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo{
        {0, createSwitchInfo(cfg::SwitchType::NPU)}};
    const std::map<int64_t, cfg::DsfNode> switchIdToDsfNodes;
    asicTable_ = std::make_unique<HwAsicTable>(
        switchIdToSwitchInfo, std::nullopt, switchIdToDsfNodes);
    scopeResolver_ =
        std::make_unique<SwitchIdScopeResolver>(switchIdToSwitchInfo);
    resourceAccountant_ = std::make_unique<ResourceAccountant>(
        asicTable_.get(), scopeResolver_.get());
    FLAGS_ecmp_width = getMaxEcmpWidth();
  }

  const std::shared_ptr<Route<folly::IPAddressV6>> makeV6Route(
      const RoutePrefix<folly::IPAddressV6>& prefix,
      const RouteNextHopEntry& entry) {
    auto route = std::make_shared<Route<folly::IPAddressV6>>(prefix);
    route->setResolved(entry);
    return route;
  }

  uint64_t getMaxEcmpWidth() {
    return asicTable_->getHwAsic(SwitchID(0))->getMaxEcmpSize();
  }
  uint64_t getMaxEcmpGroups() {
    auto maxEcmpGroups = asicTable_->getHwAsic(SwitchID(0))->getMaxEcmpGroups();
    CHECK(maxEcmpGroups.has_value());
    return maxEcmpGroups.value();
  }

  uint64_t getMaxEcmpMembers() {
    auto maxEcmpMembers =
        asicTable_->getHwAsic(SwitchID(0))->getMaxEcmpMembers();
    CHECK(maxEcmpMembers.has_value());
    return maxEcmpMembers.value();
  }

  uint64_t getMaxDlbEcmpGroups() {
    auto maxDlbEcmpGroups =
        asicTable_->getHwAsic(SwitchID(0))->getMaxDlbEcmpGroups();
    CHECK(maxDlbEcmpGroups.has_value());
    return maxDlbEcmpGroups.value();
  }

  std::unique_ptr<ResourceAccountant> resourceAccountant_;
  std::unique_ptr<HwAsicTable> asicTable_;
  std::unique_ptr<SwitchIdScopeResolver> scopeResolver_;
};

TEST_F(ResourceAccountantTest, getMemberCountForEcmpGroup) {
  const auto ecmpWidth = (getMaxEcmpWidth() / 2) + 1;

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

TEST_F(ResourceAccountantTest, checkArsResource) {
  FLAGS_dlbResourceCheckEnable = true;
  FLAGS_flowletSwitchingEnable = true;
  // MockAsic is configured to support 7 DLB groups
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));

  auto addEcmp = [](int i, std::vector<RouteNextHopSet>& ecmpList) {
    ecmpList.push_back(RouteNextHopSet{
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
            InterfaceID(i + 1),
            ecmpWeight),
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 2)),
            InterfaceID(i + 2),
            ecmpWeight)});
  };
  int i = 0;
  std::vector<RouteNextHopSet> ecmpNexthopsList;
  ecmpNexthopsList.reserve(getMaxDlbEcmpGroups());
  for (i = 0; i < getMaxDlbEcmpGroups() - 2; i++) {
    addEcmp(i, ecmpNexthopsList);
  }
  for (const auto& nhopSet : ecmpNexthopsList) {
    this->resourceAccountant_->arsEcmpGroupRefMap_[nhopSet] = 1;
  }
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      false /* intermediateState */));

  // add 1 more should take it over dlb resource percentage
  addEcmp(i++, ecmpNexthopsList);
  for (const auto& nhopSet : ecmpNexthopsList) {
    this->resourceAccountant_->arsEcmpGroupRefMap_[nhopSet] = 1;
  }
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkArsResource(
      false /* intermediateState */));

  // add 2 more should take it over 100%
  addEcmp(i++, ecmpNexthopsList);
  addEcmp(i++, ecmpNexthopsList);
  for (const auto& nhopSet : ecmpNexthopsList) {
    this->resourceAccountant_->arsEcmpGroupRefMap_[nhopSet] = 1;
  }
  EXPECT_FALSE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkArsResource(
      false /* intermediateState */));
}

TEST_F(ResourceAccountantTest, checkEcmpResource) {
  // MockAsic is configured to support 20 group and 128 members.
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add one ECMP group with max ECMP membere
  const auto ecmpWidth = getMaxEcmpWidth();

  // Add 3 groups of max ecmp width. This consumes 75% of ECMP members
  std::vector<RouteNextHopSet> ecmpNexthops;
  for (int i = 0; i < 3; ++i) {
    RouteNextHopSet ecmpNexthopsCur;
    for (int j = 0; j < ecmpWidth; j++) {
      ecmpNexthopsCur.insert(ResolvedNextHop(
          folly::IPAddress(folly::to<std::string>(i + 1, ".1.1.", j + 1)),
          InterfaceID(i + 1),
          ecmpWeight));
    }
    this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthopsCur] = 1;
    this->resourceAccountant_->ecmpMemberUsage_ += ecmpWidth;
    ecmpNexthops.push_back(std::move(ecmpNexthopsCur));
  }
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add a ecmp groups with 2 members. So resource usage now exceeds
  // 75% of max ecmp members

  RouteNextHopSet ecmpNexthops4;
  for (int i = 0; i < 2; i++) {
    ecmpNexthops4.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("100.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight));
  }
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops4] = 1;
  this->resourceAccountant_->ecmpMemberUsage_ += 2;
  // Intermediate limit is 100%, which is not violated
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  // End limit is 75% which is now violated
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add another ECMP group with max ECMP width
  RouteNextHopSet ecmpNexthops5;
  for (int i = 0; i < ecmpWidth; i++) {
    ecmpNexthops4.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("4.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight));
  }
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops5] = 1;
  this->resourceAccountant_->ecmpMemberUsage_ += ecmpWidth;
  // Both intermediate and end limit are violated
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Remove ecmpGroup4, 5:
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops4);
  this->resourceAccountant_->ecmpMemberUsage_ -= 2;
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops5);
  this->resourceAccountant_->ecmpMemberUsage_ -= ecmpWidth;
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add three groups (with width = 2 each) and remove ecmpGroup0
  std::vector<RouteNextHopSet> ecmpNexthopsList;
  ecmpNexthopsList.reserve(
      (getMaxEcmpGroups() * FLAGS_ecmp_resource_percentage / 100.0));
  for (int i = ecmpNexthops.size();
       i < (getMaxEcmpGroups() * FLAGS_ecmp_resource_percentage / 100.0);
       i++) {
    ecmpNexthopsList.push_back(RouteNextHopSet{
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
            InterfaceID(i + 1),
            ecmpWeight),
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 2)),
            InterfaceID(i + 2),
            ecmpWeight)});
  }
  for (const auto& nhopSet : ecmpNexthopsList) {
    this->resourceAccountant_->ecmpGroupRefMap_[nhopSet] = 1;
    this->resourceAccountant_->ecmpMemberUsage_ += 2;
  }
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops[0]);
  this->resourceAccountant_->ecmpMemberUsage_ -= ecmpWidth;
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add one more group to exceed group limit
  RouteNextHopSet ecmpNexthops2 = RouteNextHopSet{
      ResolvedNextHop(
          folly::IPAddress(folly::to<std::string>("3.1.1.1")),
          InterfaceID(1),
          ecmpWeight),
      ResolvedNextHop(
          folly::IPAddress(folly::to<std::string>("3.1.1.2")),
          InterfaceID(2),
          ecmpWeight)};
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops2] = 1;
  this->resourceAccountant_->ecmpMemberUsage_ += 2;
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));
}

TEST_F(ResourceAccountantTest, checkAndUpdateGenericEcmpResource) {
  // Add one ECMP group with ECMP width limit
  const auto ecmpWidth = getMaxEcmpWidth();
  auto constexpr kEcmpGroupsOfHalfWidth = 7;
  std::array<RouteNextHopSet, kEcmpGroupsOfHalfWidth> ecmpNexthops;
  std::array<std::shared_ptr<RouteV6>, kEcmpGroupsOfHalfWidth> routes;
  for (int i = 0; i < kEcmpGroupsOfHalfWidth; ++i) {
    for (int j = 0; j < ecmpWidth / 2; j++) {
      ecmpNexthops[i].insert(ResolvedNextHop(
          folly::IPAddress(folly::to<std::string>(i + 1, "::", j + 1)),
          InterfaceID(j + 1),
          ecmpWeight));
    }
    routes[i] = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("100::", i + 1)), 128},
        {ecmpNexthops[i], AdminDistance::EBGP});
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        routes[i], true /* add */));
  }

  // Add another ECMP group with max ECMP width
  RouteNextHopSet ecmpNexthops8;
  for (int i = 0; i < ecmpWidth; i++) {
    ecmpNexthops8.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("2.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight));
  }
  const auto route8 = makeV6Route(
      {folly::IPAddressV6("200::1"), 128},
      {ecmpNexthops8, AdminDistance::EBGP});
  EXPECT_FALSE(this->resourceAccountant_
                   ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                       route8, true /* add */));

  // Remove above route
  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      route8, false /* add */));
  // Add more groups (with width = 2 each) and
  const auto groupsToAdd = getMaxEcmpGroups() - kEcmpGroupsOfHalfWidth + 1;
  for (int i = 0; i < groupsToAdd; i++) {
    auto ecmpNextHops = RouteNextHopSet{
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
            InterfaceID(i + 1),
            ecmpWeight),
        ResolvedNextHop(
            folly::IPAddress(folly::to<std::string>("1.1.1.", i + 2)),
            InterfaceID(i + 2),
            ecmpWeight)};
    const auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("300::", i + 1)), 128},
        {ecmpNextHops, AdminDistance::EBGP});
    if (i < groupsToAdd - 1) {
      EXPECT_TRUE(this->resourceAccountant_
                      ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                          route, true /* add */));
    } else {
      EXPECT_FALSE(this->resourceAccountant_
                       ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                           route, true /* add */));
    }
  }

  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      routes[0], false /* add */));
}

TEST_F(ResourceAccountantTest, checkAndUpdateArsEcmpResource) {
  FLAGS_dlbResourceCheckEnable = true;
  FLAGS_flowletSwitchingEnable = true;
  FLAGS_enable_ecmp_resource_manager = true;
  auto addRoute =
      [this](
          int i,
          std::vector<std::shared_ptr<Route<folly::IPAddressV6>>>& routes,
          std::optional<cfg::SwitchingMode> switchingMode) {
        auto ecmpNextHops = RouteNextHopSet{
            ResolvedNextHop(
                folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
                InterfaceID(i + 1),
                ecmpWeight),
            ResolvedNextHop(
                folly::IPAddress(folly::to<std::string>("1.1.1.", i + 2)),
                InterfaceID(i + 2),
                ecmpWeight)};
        const RouteNextHopEntry routeNextHopEntry{
            ecmpNextHops,
            AdminDistance::EBGP,
            std::optional<RouteCounterID>(std::nullopt),
            std::optional<cfg::AclLookupClass>(std::nullopt),
            switchingMode};
        auto route = makeV6Route(
            {folly::IPAddressV6(folly::to<std::string>(i + 1, "00::1")), 128},
            routeNextHopEntry);
        routes.push_back(std::move(route));
      };
  int i = 0;
  std::vector<std::shared_ptr<Route<folly::IPAddressV6>>> routes;
  // add upto limit of DLB groups
  for (i = 0; i < getMaxDlbEcmpGroups(); i++) {
    addRoute(i, routes, std::nullopt);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateArsEcmpResource<folly::IPAddressV6>(
                        routes.back(), true /* add */));
  }

  // add backup ECMP groups, still should be OK
  for (; i < 10; i++) {
    addRoute(i, routes, cfg::SwitchingMode::PER_PACKET_RANDOM);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateArsEcmpResource<folly::IPAddressV6>(
                        routes.back(), true /* add */));
  }
}

TEST_F(ResourceAccountantTest, computeWeightedEcmpMemberCount) {
  RouteNextHopSet ecmpNexthops;
  for (int i = 0; i < getMaxEcmpMembers() - 1; i++) {
    ecmpNexthops.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("1::", i + 1)),
        InterfaceID(i + 1),
        i + 1));
  }
  RouteNextHopEntry ecmpNextHopEntry =
      RouteNextHopEntry(ecmpNexthops, AdminDistance::EBGP);
  EXPECT_EQ(
      this->resourceAccountant_->computeWeightedEcmpMemberCount(
          ecmpNextHopEntry, cfg::AsicType::ASIC_TYPE_TOMAHAWK4),
      4 * ecmpNexthops.size());
  // Assume ECMP replication for devices without specifying UCMP computation.
  uint32_t totalWeight = 0;
  for (const auto& nhop : ecmpNextHopEntry.normalizedNextHops()) {
    totalWeight += nhop.weight() ? nhop.weight() : 1;
    XLOG(INFO) << "Weighted ECMP member count: " << nhop.weight();
  }
  EXPECT_EQ(
      this->resourceAccountant_->computeWeightedEcmpMemberCount(
          ecmpNextHopEntry, cfg::AsicType::ASIC_TYPE_MOCK),
      totalWeight);
}

} // namespace facebook::fboss
