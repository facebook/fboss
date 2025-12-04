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
} // namespace

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

  uint64_t getMaxArsGroups() {
    auto maxArsGroups = asicTable_->getHwAsic(SwitchID(0))->getMaxArsGroups();
    CHECK(maxArsGroups.has_value());
    return maxArsGroups.value();
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
    ecmpList.push_back(
        RouteNextHopSet{
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
  ecmpNexthopsList.reserve(getMaxArsGroups());
  for (i = 0; i < getMaxArsGroups() - 2; i++) {
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
    ecmpNexthopsList.push_back(
        RouteNextHopSet{
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

TEST_F(ResourceAccountantTest, checkEcmpResourceForUcmpWeights) {
  // MockAsic is configured to support 20 group and 128 members.
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  const auto ecmpWidth = getMaxEcmpWidth();

  // Add 3 groups of max ecmp width. This consumes 75% of UCMP members
  std::vector<RouteNextHopSet> ecmpNexthops =
      getUcmpNextHops(ecmpWidth, 3 /*numGroups*/, 0 /*seed*/);
  for (const auto& ecmpNexthopsCur : ecmpNexthops) {
    this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthopsCur] = 1;
    this->resourceAccountant_->ecmpMemberUsage_ += ecmpWidth;
  }
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add a ucmp groups with 2 members. So resource usage now exceeds
  // 75% of max ucmp members
  auto singleGroup = getUcmpNextHops(ecmpWidth, 1 /*numGroups*/, 1 /*seed*/);
  RouteNextHopSet ecmpNexthops2 = singleGroup[0];

  this->resourceAccountant_->ecmpMemberUsage_ += 5;
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops2] = 1;
  // Intermediate limit is 100%, which is not violated
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  // End limit is 75% which is now violated
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add another UCMP group with max ECMP width
  auto singleGroup2 = getUcmpNextHops(ecmpWidth, 1 /*numGroups*/, 2 /*seed*/);
  RouteNextHopSet ecmpNexthops3 = singleGroup2[0];
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops3] = 1;
  this->resourceAccountant_->ecmpMemberUsage_ += ecmpWidth;
  // Both intermediate and end limit are violated
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Remove ecmpGroup4, 5:
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops2);
  this->resourceAccountant_->ecmpMemberUsage_ -= 5;
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops3);
  this->resourceAccountant_->ecmpMemberUsage_ -= ecmpWidth;
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add more 13 more groups (with width = 5 each) and remove ucmpGroup0
  std::vector<RouteNextHopSet> ecmpNexthopsList;
  ecmpNexthopsList.reserve(
      (getMaxEcmpGroups() * FLAGS_ecmp_resource_percentage / 100.0));
  ecmpNexthopsList = getUcmpNextHops(5, 13 /*numGroups*/, 3 /*seed*/);
  for (const auto& nhopSet : ecmpNexthopsList) {
    this->resourceAccountant_->ecmpGroupRefMap_[nhopSet] = 1;
    this->resourceAccountant_->ecmpMemberUsage_ += 5;
  }
  this->resourceAccountant_->ecmpGroupRefMap_.erase(ecmpNexthops[0]);
  this->resourceAccountant_->ecmpMemberUsage_ -= ecmpWidth;

  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add one more group to exceed group limit
  auto singleGroup3 = getUcmpNextHops(ecmpWidth, 1 /*numGroups*/, 4 /*seed*/);
  RouteNextHopSet ecmpNexthops4 = singleGroup3[0];
  this->resourceAccountant_->ecmpGroupRefMap_[ecmpNexthops4] = 1;
  this->resourceAccountant_->ecmpMemberUsage_ += 5;

  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));
  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
      false /* intermediateState */));

  // Add 5 more groups with 2 members each and width 5. This consumes 100% of
  // UCMP groups
  std::vector<RouteNextHopSet> ecmpNexthopsListFinal =
      getUcmpNextHops(5, 5 /*numGroups*/, 5 /*seed*/);

  for (const auto& nhopSet : ecmpNexthopsListFinal) {
    this->resourceAccountant_->ecmpGroupRefMap_[nhopSet] = 1;
    this->resourceAccountant_->ecmpMemberUsage_ += 5;
  }

  EXPECT_FALSE(this->resourceAccountant_->checkEcmpResource(
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

TEST_F(
    ResourceAccountantTest,
    checkAndUpdateGenericEcmpResourceForUcmpWeights) {
  // Add one ECMP group with ECMP width limit
  const auto ecmpWidth = getMaxEcmpWidth();
  auto constexpr kUcmpGroupsWithWeights = 7;
  auto constexpr kTotalWeightForUCMP = 60;
  std::array<RouteNextHopSet, kUcmpGroupsWithWeights> ecmpNexthops;
  std::array<std::shared_ptr<RouteV6>, kUcmpGroupsWithWeights> routes;

  // Configure kUcmpGroupsWithWeights number of UCMP groups with
  // kTotalWeightForUCMP each
  std::vector<RouteNextHopSet> ecmpNextHops = getUcmpNextHops(
      kTotalWeightForUCMP, kUcmpGroupsWithWeights /*numGroups*/, 0 /*seed*/);
  for (int i = 0; i < kUcmpGroupsWithWeights; i++) {
    ecmpNexthops[i] = ecmpNextHops[i]; // Copy the generated groups
    routes[i] = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("100::", i + 1)), 128},
        {ecmpNexthops[i], AdminDistance::EBGP});

    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                        routes[i], true /* add */));
  }

  // Add another UCMP group where total weights of all members in all groups  >
  // maxEcmpMembers
  auto singleGroup8 = getUcmpNextHops(ecmpWidth, 1 /*numGroups*/, 1 /*seed*/);
  RouteNextHopSet ecmpNexthops8 = singleGroup8[0];
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

  //  Add more groups (with width = 5 each) and
  // Testing max UCMP groups > MaxEcmpGroups returns False.
  const auto groupsToAdd = getMaxEcmpGroups() - kUcmpGroupsWithWeights + 1;
  std::vector<RouteNextHopSet> ecmpNextHopsRemaining =
      getUcmpNextHops(5, groupsToAdd /*numGroups*/, 2 /*seed*/);
  for (int i = 0; i < groupsToAdd; i++) {
    const auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("300::", i + 1)), 128},
        {ecmpNextHopsRemaining[i], AdminDistance::EBGP});

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
  for (i = 0; i < getMaxArsGroups(); i++) {
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
      4 * ecmpNextHopEntry.normalizedNextHops().size());
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

TEST_F(ResourceAccountantTest, checkNeighborResource) {
  auto asicNdpMax =
      asicTable_->getHwAsic(SwitchID(0))->getMaxNdpTableSize().value();
  auto asicArpMax =
      asicTable_->getHwAsic(SwitchID(0))->getMaxArpTableSize().value();
  // asicUnifiedNeighborMax is set to <  (asicNdpMax + asicArpMax)  and >
  // (asicNdpMax or asicArpMax)
  auto asicUnifiedNeighborMax = asicTable_->getHwAsic(SwitchID(0))
                                    ->getMaxUnifiedNeighborTableSize()
                                    .value();
  auto& ndpEntriesMap = this->resourceAccountant_->ndpEntriesMap_;
  auto& arpEntriesMap = this->resourceAccountant_->arpEntriesMap_;

  // Test with configured max neighbor entries
  FLAGS_max_ndp_entries = asicNdpMax + 100;
  FLAGS_max_arp_entries = asicArpMax + 100;
  FLAGS_enforce_resource_hw_limits = false;
  ndpEntriesMap.insert_or_assign(SwitchID(0), FLAGS_max_ndp_entries);
  arpEntriesMap.insert_or_assign(SwitchID(0), FLAGS_max_arp_entries);
  EXPECT_TRUE(this->resourceAccountant_->checkNeighborResource());
  ndpEntriesMap.insert_or_assign(SwitchID(0), FLAGS_max_ndp_entries + 1);
  EXPECT_FALSE(this->resourceAccountant_->checkNeighborResource());
  ndpEntriesMap.insert_or_assign(SwitchID(0), FLAGS_max_ndp_entries);
  arpEntriesMap.insert_or_assign(SwitchID(0), FLAGS_max_arp_entries + 1);
  EXPECT_FALSE(this->resourceAccountant_->checkNeighborResource());

  // check with ASIC limits
  FLAGS_enforce_resource_hw_limits = true;
  ndpEntriesMap.insert_or_assign(SwitchID(0), asicNdpMax);
  arpEntriesMap.insert_or_assign(
      SwitchID(0), asicUnifiedNeighborMax - asicArpMax);
  EXPECT_TRUE(this->resourceAccountant_->checkNeighborResource());
  arpEntriesMap.insert_or_assign(
      SwitchID(0), asicUnifiedNeighborMax - asicArpMax + 1);
  EXPECT_FALSE(this->resourceAccountant_->checkNeighborResource());
  ndpEntriesMap.insert_or_assign(SwitchID(0), asicNdpMax + 1);
  arpEntriesMap.clear();
  EXPECT_FALSE(this->resourceAccountant_->checkNeighborResource());
  arpEntriesMap.insert_or_assign(SwitchID(0), asicArpMax + 1);
  ndpEntriesMap.clear();
  EXPECT_FALSE(this->resourceAccountant_->checkNeighborResource());
}

TEST_F(ResourceAccountantTest, routeWithAdjustedWeightZero) {
  // Create route with ECMP next hops where one has adjustedWeight of 0
  // The next hop with adjustedWeight 0 should not be counted towards ECMP
  // member usage since ResourceAccountant tracks normalized next hops

  // Create ECMP next hops: 3 next hops total, but one with adjustedWeight=0
  RouteNextHopSet ecmpNexthops;
  // Next hop 1: normal ECMP member with weight 1
  ecmpNexthops.insert(ResolvedNextHop(
      folly::IPAddress("1.1.1.1"),
      InterfaceID(1),
      ecmpWeight,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt));

  // Next hop 2: normal ECMP member with weight 1
  ecmpNexthops.insert(ResolvedNextHop(
      folly::IPAddress("1.1.1.2"),
      InterfaceID(2),
      ecmpWeight,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt));

  // Next hop 3: ECMP member with adjustedWeight=0 (should be filtered out)
  ecmpNexthops.insert(ResolvedNextHop(
      folly::IPAddress("1.1.1.3"),
      InterfaceID(3),
      ecmpWeight,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      NextHopWeight(0)));

  RouteNextHopEntry ecmpNextHopEntry =
      RouteNextHopEntry(ecmpNexthops, AdminDistance::EBGP);

  // Only 2 next hops should be counted (the one with adjustedWeight=0 is
  // filtered)
  EXPECT_EQ(
      2,
      this->resourceAccountant_->getMemberCountForEcmpGroup(ecmpNextHopEntry));

  // Add route with the ECMP next hop entry
  auto route =
      makeV6Route({folly::IPAddressV6("2001:db8::1"), 128}, ecmpNextHopEntry);

  auto initialEcmpMemberUsage = this->resourceAccountant_->ecmpMemberUsage_;

  // Validate the route via ResourceAccountant
  EXPECT_TRUE(
      this->resourceAccountant_->checkAndUpdateEcmpResource(route, true));
  // Check that we tracked only 2 ECMP members (not 3)
  EXPECT_EQ(
      initialEcmpMemberUsage + 2, this->resourceAccountant_->ecmpMemberUsage_);

  // Test removing the route
  EXPECT_TRUE(
      this->resourceAccountant_->checkAndUpdateEcmpResource(route, false));
  EXPECT_EQ(
      initialEcmpMemberUsage, this->resourceAccountant_->ecmpMemberUsage_);
}

// This test verifies that routeAndEcmpStateChangedImpl correctly handles
// resolved and unresolved routes.
// 1. Add resolved and unresolved routes - only resolved are tracked
// 2. Delete resolved and unresolved routes - only resolved affect usage
// 3. Change resolved routes to unresolved - remove from tracking
// 4. Change unresolved routes to resolved - add to tracking
TEST_F(ResourceAccountantTest, resolvedAndUnresolvedRoutes) {
  FLAGS_enable_route_resource_protection = true;

  // Create nexthops for resolved routes
  RouteNextHopSet ecmpNexthops;
  ecmpNexthops.insert(
      ResolvedNextHop(folly::IPAddress("1.1.1.1"), InterfaceID(1), ecmpWeight));
  ecmpNexthops.insert(
      ResolvedNextHop(folly::IPAddress("1.1.1.2"), InterfaceID(2), ecmpWeight));
  RouteNextHopEntry ecmpNextHopEntry =
      RouteNextHopEntry(ecmpNexthops, AdminDistance::EBGP);

  auto initialEcmpMemberUsage = this->resourceAccountant_->ecmpMemberUsage_;
  auto initialEcmpGroupCount =
      this->resourceAccountant_->ecmpGroupRefMap_.size();
  auto initialRouteUsage = this->resourceAccountant_->routeUsage_;

  // Helper to build SwitchState with V6 routes
  auto makeSwitchStateWithV6Routes =
      [&](const std::vector<std::shared_ptr<RouteV6>>& routes) {
        auto state = std::make_shared<SwitchState>();
        auto fibContainer =
            std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
        ForwardingInformationBaseV6 fibV6;

        for (const auto& route : routes) {
          route->publish();
          fibV6.addNode(route);
        }

        fibContainer->setFib(fibV6.clone());

        auto fibMap =
            std::make_shared<MultiSwitchForwardingInformationBaseMap>();
        fibMap->addNode(fibContainer, HwSwitchMatcher());
        state->resetForwardingInformationBases(fibMap);
        return state;
      };

  // Test 1: Add resolved and unresolved routes
  // Only resolved routes should increase usage
  auto resolvedRoute1 =
      makeV6Route({folly::IPAddressV6("100::1"), 128}, ecmpNextHopEntry);
  auto resolvedRoute2 =
      makeV6Route({folly::IPAddressV6("100::2"), 128}, ecmpNextHopEntry);

  // Create unresolved route
  auto unresolvedRoute1 = std::make_shared<RouteV6>(RouteV6::makeThrift(
      RoutePrefix<folly::IPAddressV6>{folly::IPAddressV6("200::1"), 128}));

  auto oldState1 = makeSwitchStateWithV6Routes({});
  auto newState1 = makeSwitchStateWithV6Routes(
      {resolvedRoute1, resolvedRoute2, unresolvedRoute1});
  StateDelta delta1(oldState1, newState1);

  EXPECT_TRUE(this->resourceAccountant_->routeAndEcmpStateChangedImpl(delta1));

  // Only 2 resolved routes should be counted
  EXPECT_EQ(this->resourceAccountant_->routeUsage_, initialRouteUsage + 2);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpGroupRefMap_.size(),
      initialEcmpGroupCount + 1);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpMemberUsage_, initialEcmpMemberUsage + 2);

  // Test 2: Delete resolved and unresolved routes
  // Only resolved route deletions should decrease usage
  auto oldState2 = newState1;
  auto newState2 = makeSwitchStateWithV6Routes({});
  StateDelta delta2(oldState2, newState2);

  EXPECT_TRUE(this->resourceAccountant_->routeAndEcmpStateChangedImpl(delta2));

  // Should go back to initial state
  EXPECT_EQ(this->resourceAccountant_->routeUsage_, initialRouteUsage);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpGroupRefMap_.size(),
      initialEcmpGroupCount);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpMemberUsage_, initialEcmpMemberUsage);

  // Test 3: Change resolved routes to unresolved
  // First add resolved routes
  auto oldState3a = makeSwitchStateWithV6Routes({});
  auto newState3a =
      makeSwitchStateWithV6Routes({resolvedRoute1, resolvedRoute2});
  StateDelta delta3a(oldState3a, newState3a);
  EXPECT_TRUE(this->resourceAccountant_->routeAndEcmpStateChangedImpl(delta3a));

  auto usageBeforeUnresolve = this->resourceAccountant_->routeUsage_;
  auto ecmpUsageBeforeUnresolve = this->resourceAccountant_->ecmpMemberUsage_;

  // Change to unresolved (same prefixes, but unresolved)
  auto unresolvedRoute2 = std::make_shared<RouteV6>(RouteV6::makeThrift(
      RoutePrefix<folly::IPAddressV6>{folly::IPAddressV6("100::1"), 128}));
  auto unresolvedRoute3 = std::make_shared<RouteV6>(RouteV6::makeThrift(
      RoutePrefix<folly::IPAddressV6>{folly::IPAddressV6("100::2"), 128}));

  auto oldState3b = newState3a;
  auto newState3b =
      makeSwitchStateWithV6Routes({unresolvedRoute2, unresolvedRoute3});
  StateDelta delta3b(oldState3b, newState3b);
  EXPECT_TRUE(this->resourceAccountant_->routeAndEcmpStateChangedImpl(delta3b));

  // Usage should decrease when routes become unresolved
  EXPECT_EQ(this->resourceAccountant_->routeUsage_, usageBeforeUnresolve - 2);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpMemberUsage_,
      ecmpUsageBeforeUnresolve - 2);

  // Test 4: Change unresolved routes to resolved
  auto usageBeforeResolve = this->resourceAccountant_->routeUsage_;
  auto ecmpUsageBeforeResolve = this->resourceAccountant_->ecmpMemberUsage_;

  auto oldState4 = newState3b;
  auto newState4 =
      makeSwitchStateWithV6Routes({resolvedRoute1, resolvedRoute2});
  StateDelta delta4(oldState4, newState4);
  EXPECT_TRUE(this->resourceAccountant_->routeAndEcmpStateChangedImpl(delta4));

  // Usage should increase when routes become resolved
  EXPECT_EQ(this->resourceAccountant_->routeUsage_, usageBeforeResolve + 2);
  EXPECT_EQ(
      this->resourceAccountant_->ecmpMemberUsage_, ecmpUsageBeforeResolve + 2);
}

} // namespace facebook::fboss
