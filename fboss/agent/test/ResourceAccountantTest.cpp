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
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

namespace {
constexpr auto ecmpWeight = 1;
} // namespace

namespace facebook::fboss {

class ResourceAccountantTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo{
        {0, createSwitchInfo(cfg::SwitchType::NPU)}};
    const std::map<int64_t, cfg::DsfNode> switchIdToDsfNodes;
    asicTable_ = std::make_unique<HwAsicTable>(
        switchIdToSwitchInfo, std::nullopt, switchIdToDsfNodes);
    scopeResolver_ =
        std::make_unique<SwitchIdScopeResolver>(switchIdToSwitchInfo);
    resourceAccountant_ = std::make_unique<ResourceAccountant>(
        asicTable_.get(), scopeResolver_.get());
    nextHopIDManager_ = std::make_unique<NextHopIDManager>();
    FLAGS_ecmp_width = getMaxEcmpWidth();
    initState();
  }
  void initState() {
    state_ = std::make_shared<SwitchState>();
    auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
    auto fibInfo = std::make_shared<FibInfo>();
    fibInfo->setIdToNextHopMap(std::make_shared<IdToNextHopMap>());
    fibInfo->setIdToNextHopIdSetMap(std::make_shared<IdToNextHopIdSetMap>());
    fibInfoMap->updateFibInfo(fibInfo, getScope());
    state_->resetFibsInfoMap(fibInfoMap);
  }

  HwSwitchMatcher getScope() const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
  }

  std::shared_ptr<FibInfo> getFibInfo() const {
    return state_->getFibsInfoMap()->getFibInfo(getScope());
  }

  void allocAndUpdateIdMaps(
      RouteNextHopEntry& entry,
      const RouteNextHopSet& nhops) {
    if (!FLAGS_enable_nexthop_id_manager) {
      return;
    }
    auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhops);
    std::optional<NextHopSetID> resolvedId =
        allocResult.nextHopIdSetIter->second.id;
    entry.setResolvedNextHopSetID(resolvedId);

    auto normalizedNhops = entry.nonOverrideNormalizedNextHops();
    auto normAllocResult =
        nextHopIDManager_->getOrAllocRouteNextHopSetID(normalizedNhops);
    std::optional<NextHopSetID> normalizedId =
        normAllocResult.nextHopIdSetIter->second.id;
    entry.setNormalizedResolvedNextHopSetID(normalizedId);

    // Incrementally add only new entries to the existing maps on state_
    auto fibInfo = getFibInfo();
    auto idToNextHopMap = fibInfo->getIdToNextHopMap();
    auto idToNextHopIdSetMap = fibInfo->getIdToNextHopIdSetMap();
    for (const auto& [id, nhop] : nextHopIDManager_->getIdToNextHop()) {
      if (!idToNextHopMap->getNextHopIf(id)) {
        idToNextHopMap->addNextHop(id, nhop.toThrift());
      }
    }
    for (const auto& [id, nhopIdSet] :
         nextHopIDManager_->getIdToNextHopIdSet()) {
      if (!idToNextHopIdSetMap->getNextHopIdSetIf(id)) {
        std::set<int64_t> nhopIds;
        for (const auto& nhopId : nhopIdSet) {
          nhopIds.insert(static_cast<int64_t>(nhopId));
        }
        idToNextHopIdSetMap->addNextHopIdSet(id, nhopIds);
      }
    }
  }

  const std::shared_ptr<Route<folly::IPAddressV6>> makeV6Route(
      const RoutePrefix<folly::IPAddressV6>& prefix,
      const RouteNextHopEntry& entry,
      const RouteNextHopSet& nhops) {
    // Clone via thrift round-trip (copy ctor is deleted), allocate IDs
    RouteNextHopEntry mutableEntry;
    mutableEntry.fromThrift(entry.toThrift());
    allocAndUpdateIdMaps(mutableEntry, nhops);
    auto route = std::make_shared<Route<folly::IPAddressV6>>(prefix);
    route->setResolved(mutableEntry);
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
  std::unique_ptr<NextHopIDManager> nextHopIDManager_;
  std::shared_ptr<SwitchState> state_;
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
  this->allocAndUpdateIdMaps(ecmpNextHopEntry, ecmpNexthops);
  EXPECT_EQ(
      ecmpWidth,
      this->resourceAccountant_->getMemberCountForEcmpGroup(
          ecmpNextHopEntry, state_));

  RouteNextHopSet ucmpNexthops;
  for (int i = 0; i < ecmpWidth; i++) {
    ucmpNexthops.insert(ResolvedNextHop(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        i + 1));
  }
  RouteNextHopEntry ucmpNextHopEntry =
      RouteNextHopEntry(ucmpNexthops, AdminDistance::EBGP);
  this->allocAndUpdateIdMaps(ucmpNextHopEntry, ucmpNexthops);
  auto expectedTotalMember = 0;
  for (const auto& nhop : ucmpNextHopEntry.normalizedNextHops()) {
    expectedTotalMember += nhop.weight();
  }
  EXPECT_EQ(
      expectedTotalMember,
      this->resourceAccountant_->getMemberCountForEcmpGroup(
          ucmpNextHopEntry, state_));
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
        {ecmpNexthops[i], AdminDistance::EBGP},
        ecmpNexthops[i]);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        routes[i], true /* add */, state_));
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
      {ecmpNexthops8, AdminDistance::EBGP},
      ecmpNexthops8);
  EXPECT_FALSE(this->resourceAccountant_
                   ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                       route8, true /* add */, state_));

  // Remove above route
  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      route8, false /* add */, state_));
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
        {ecmpNextHops, AdminDistance::EBGP},
        ecmpNextHops);
    if (i < groupsToAdd - 1) {
      EXPECT_TRUE(this->resourceAccountant_
                      ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                          route, true /* add */, state_));
    } else {
      EXPECT_FALSE(this->resourceAccountant_
                       ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                           route, true /* add */, state_));
    }
  }

  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      routes[0], false /* add */, state_));
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
        {ecmpNexthops[i], AdminDistance::EBGP},
        ecmpNexthops[i]);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                        routes[i], true /* add */, state_));
  }

  // Add another UCMP group where total weights of all members in all groups  >
  // maxEcmpMembers
  auto singleGroup8 = getUcmpNextHops(ecmpWidth, 1 /*numGroups*/, 1 /*seed*/);
  RouteNextHopSet ecmpNexthops8 = singleGroup8[0];
  const auto route8 = makeV6Route(
      {folly::IPAddressV6("200::1"), 128},
      {ecmpNexthops8, AdminDistance::EBGP},
      ecmpNexthops8);
  EXPECT_FALSE(this->resourceAccountant_
                   ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                       route8, true /* add */, state_));

  // Remove above route
  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      route8, false /* add */, state_));

  //  Add more groups (with width = 5 each) and
  // Testing max UCMP groups > MaxEcmpGroups returns False.
  const auto groupsToAdd = getMaxEcmpGroups() - kUcmpGroupsWithWeights + 1;
  std::vector<RouteNextHopSet> ecmpNextHopsRemaining =
      getUcmpNextHops(5, groupsToAdd /*numGroups*/, 2 /*seed*/);
  for (int i = 0; i < groupsToAdd; i++) {
    const auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("300::", i + 1)), 128},
        {ecmpNextHopsRemaining[i], AdminDistance::EBGP},
        ecmpNextHopsRemaining[i]);
    if (i < groupsToAdd - 1) {
      EXPECT_TRUE(this->resourceAccountant_
                      ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                          route, true /* add */, state_));
    } else {
      EXPECT_FALSE(this->resourceAccountant_
                       ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                           route, true /* add */, state_));
    }
  }

  EXPECT_TRUE(this->resourceAccountant_
                  ->checkAndUpdateGenericEcmpResource<folly::IPAddressV6>(
                      routes[0], false /* add */, state_));
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
            routeNextHopEntry,
            ecmpNextHops);
        routes.push_back(std::move(route));
      };
  int i = 0;
  std::vector<std::shared_ptr<Route<folly::IPAddressV6>>> routes;
  // add upto limit of DLB groups
  for (i = 0; i < getMaxArsGroups(); i++) {
    addRoute(i, routes, std::nullopt);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateArsEcmpResource<folly::IPAddressV6>(
                        routes.back(), true /* add */, state_));
  }

  // add backup ECMP groups, still should be OK
  for (; i < 10; i++) {
    addRoute(i, routes, cfg::SwitchingMode::PER_PACKET_RANDOM);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateArsEcmpResource<folly::IPAddressV6>(
                        routes.back(), true /* add */, state_));
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
  this->allocAndUpdateIdMaps(ecmpNextHopEntry, ecmpNexthops);
  EXPECT_EQ(
      this->resourceAccountant_->computeWeightedEcmpMemberCount(
          ecmpNextHopEntry, cfg::AsicType::ASIC_TYPE_TOMAHAWK4, state_),
      4 * ecmpNextHopEntry.normalizedNextHops().size());
  // Assume ECMP replication for devices without specifying UCMP computation.
  uint32_t totalWeight = 0;
  for (const auto& nhop : ecmpNextHopEntry.normalizedNextHops()) {
    totalWeight += nhop.weight() ? nhop.weight() : 1;
    XLOG(INFO) << "Weighted ECMP member count: " << nhop.weight();
  }
  EXPECT_EQ(
      this->resourceAccountant_->computeWeightedEcmpMemberCount(
          ecmpNextHopEntry, cfg::AsicType::ASIC_TYPE_MOCK, state_),
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
  this->allocAndUpdateIdMaps(ecmpNextHopEntry, ecmpNexthops);

  // Only 2 next hops should be counted (the one with adjustedWeight=0 is
  // filtered)
  EXPECT_EQ(
      2,
      this->resourceAccountant_->getMemberCountForEcmpGroup(
          ecmpNextHopEntry, state_));

  // Add route with the ECMP next hop entry
  auto route = makeV6Route(
      {folly::IPAddressV6("2001:db8::1"), 128}, ecmpNextHopEntry, ecmpNexthops);

  auto initialEcmpMemberUsage = this->resourceAccountant_->ecmpMemberUsage_;

  // Validate the route via ResourceAccountant
  EXPECT_TRUE(this->resourceAccountant_->checkAndUpdateEcmpResource(
      route, true, state_));
  // Check that we tracked only 2 ECMP members (not 3)
  EXPECT_EQ(
      initialEcmpMemberUsage + 2, this->resourceAccountant_->ecmpMemberUsage_);

  // Test removing the route
  EXPECT_TRUE(this->resourceAccountant_->checkAndUpdateEcmpResource(
      route, false, state_));
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
      [this](const std::vector<std::shared_ptr<RouteV6>>& routes) {
        auto state = std::make_shared<SwitchState>();
        auto fibContainer =
            std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
        ForwardingInformationBaseV6 fibV6;

        for (const auto& route : routes) {
          route->publish();
          fibV6.addNode(route);
        }

        fibContainer->setFib(fibV6.clone());

        // Create ForwardingInformationBaseMap and add the container
        auto fibsMap = std::make_shared<ForwardingInformationBaseMap>();
        fibsMap->updateForwardingInformationBaseContainer(fibContainer);

        HwSwitchMatcher scope(std::unordered_set<SwitchID>({SwitchID(0)}));
        auto fibInfo = std::make_shared<FibInfo>();
        fibInfo->ref<switch_state_tags::fibsMap>() = fibsMap;

        // Populate ID maps for ID-based resolution
        auto id2Nhop = std::make_shared<IdToNextHopMap>();
        for (const auto& [id, nhop] : nextHopIDManager_->getIdToNextHop()) {
          id2Nhop->addNextHop(id, nhop.toThrift());
        }
        auto id2NhopSetIds = std::make_shared<IdToNextHopIdSetMap>();
        for (const auto& [id, nhopIdSet] :
             nextHopIDManager_->getIdToNextHopIdSet()) {
          std::set<int64_t> nhopIds;
          for (const auto& nhopId : nhopIdSet) {
            nhopIds.insert(static_cast<int64_t>(nhopId));
          }
          id2NhopSetIds->addNextHopIdSet(id, nhopIds);
        }
        fibInfo->setIdToNextHopMap(id2Nhop);
        fibInfo->setIdToNextHopIdSetMap(id2NhopSetIds);

        auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
        fibInfoMap->updateFibInfo(fibInfo, scope);
        state->resetFibsInfoMap(fibInfoMap);
        return state;
      };

  // Test 1: Add resolved and unresolved routes
  // Only resolved routes should increase usage
  auto resolvedRoute1 = makeV6Route(
      {folly::IPAddressV6("100::1"), 128}, ecmpNextHopEntry, ecmpNexthops);
  auto resolvedRoute2 = makeV6Route(
      {folly::IPAddressV6("100::2"), 128}, ecmpNextHopEntry, ecmpNexthops);

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

TEST_F(ResourceAccountantTest, virtualArsGroups) {
  FLAGS_dlbResourceCheckEnable = true;
  FLAGS_flowletSwitchingEnable = true;
  this->resourceAccountant_->setMinWidthForArsVirtualGroup(5);
  this->resourceAccountant_->setMaxArsVirtualGroups(256);
  this->resourceAccountant_->setMaxArsVirtualGroupWidth(256);

  // Helper to create a nexthop set of given width
  auto makeNhSet = [](int width, int seed) {
    RouteNextHopSet nhSet;
    for (int i = 0; i < width; i++) {
      nhSet.insert(ResolvedNextHop(
          folly::IPAddress(
              folly::to<std::string>(
                  (seed / 250) + 1, ".", (seed % 250) + 1, ".1.", i + 1)),
          InterfaceID(i + 1),
          ecmpWeight));
    }
    return nhSet;
  };

  // Scenario (scaled to fit mock ASIC limits):
  //   5 routes use 1 shared ARS ECMP group with 4 members (non-virtual)
  //   8 routes use 1 shared ARS ECMP group with 6 members (virtual)
  //   2 routes use unique ARS ECMP groups with 3 members each (non-virtual)
  //   3 routes use unique ARS ECMP groups with 7 members each (virtual)

  // Build shared groups
  auto sharedNonVirtualNhSet = makeNhSet(4, 0);
  RouteNextHopEntry sharedNonVirtualEntry(
      sharedNonVirtualNhSet, AdminDistance::EBGP);
  this->allocAndUpdateIdMaps(sharedNonVirtualEntry, sharedNonVirtualNhSet);

  auto sharedVirtualNhSet = makeNhSet(6, 1);
  RouteNextHopEntry sharedVirtualEntry(sharedVirtualNhSet, AdminDistance::EBGP);
  this->allocAndUpdateIdMaps(sharedVirtualEntry, sharedVirtualNhSet);

  // getMemberCountForEcmpGroup: virtual returns 0, non-virtual returns width
  EXPECT_EQ(
      4,
      this->resourceAccountant_->getMemberCountForEcmpGroup(
          sharedNonVirtualEntry, state_));
  EXPECT_EQ(
      0,
      this->resourceAccountant_->getMemberCountForEcmpGroup(
          sharedVirtualEntry, state_));

  // Add 5 routes sharing 1 non-virtual group (4 members)
  for (int i = 0; i < 5; i++) {
    auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("1::", i + 1)), 128},
        sharedNonVirtualEntry,
        sharedNonVirtualNhSet);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        route, true, state_));
  }

  // Add 8 routes sharing 1 virtual group (6 members)
  for (int i = 0; i < 8; i++) {
    auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("2::", i + 1)), 128},
        sharedVirtualEntry,
        sharedVirtualNhSet);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        route, true, state_));
  }

  // Add 2 routes with unique non-virtual groups (3 members each)
  std::vector<RouteNextHopEntry> uniqueNonVirtualEntries;
  for (int i = 0; i < 2; i++) {
    auto nhSet = makeNhSet(3, i + 100);
    uniqueNonVirtualEntries.emplace_back(nhSet, AdminDistance::EBGP);
    auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("3::", i + 1)), 128},
        uniqueNonVirtualEntries.back(),
        nhSet);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        route, true, state_));
  }

  // Add 3 routes with unique virtual groups (7 members each)
  std::vector<RouteNextHopEntry> uniqueVirtualEntries;
  for (int i = 0; i < 3; i++) {
    auto nhSet = makeNhSet(7, i + 200);
    uniqueVirtualEntries.emplace_back(nhSet, AdminDistance::EBGP);
    auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("4::", i + 1)), 128},
        uniqueVirtualEntries.back(),
        nhSet);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        route, true, state_));
  }

  // ecmpGroupRefMap_: all 7 unique groups (1+1+2+3)
  EXPECT_EQ(this->resourceAccountant_->ecmpGroupRefMap_.size(), 7);

  // arsEcmpGroupRefMap_: only 3 non-virtual groups (1 shared + 2 unique)
  EXPECT_EQ(this->resourceAccountant_->arsEcmpGroupRefMap_.size(), 3);

  // virtualArsGroupCount_: 4 unique virtual groups (1 shared + 3 unique)
  EXPECT_EQ(this->resourceAccountant_->virtualArsGroupCount_, 4);

  // ecmpMemberUsage_: 4 + 2*3 = 10 (virtual groups contribute 0)
  EXPECT_EQ(this->resourceAccountant_->ecmpMemberUsage_, 10);

  // Ref counts for shared groups
  EXPECT_EQ(
      this->resourceAccountant_->ecmpGroupRefMap_[sharedNonVirtualNhSet], 5);
  EXPECT_EQ(this->resourceAccountant_->ecmpGroupRefMap_[sharedVirtualNhSet], 8);
  EXPECT_EQ(
      this->resourceAccountant_->arsEcmpGroupRefMap_[sharedNonVirtualNhSet], 5);
  EXPECT_EQ(
      this->resourceAccountant_->arsEcmpGroupRefMap_.count(sharedVirtualNhSet),
      0);

  // checkArsResource: totalDlbUsage = 3 (non-virtual) + 4 (collective) = 7
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));

  // checkEcmpResource: totalEcmpGroupUsage = 7,
  //   totalEcmpMemberUsage = 10 + 256 = 266
  EXPECT_TRUE(this->resourceAccountant_->checkEcmpResource(
      true /* intermediateState */));

  // Remove one of 5 routes sharing non-virtual group - ref count decreases
  auto removeRoute1 = makeV6Route(
      {folly::IPAddressV6("1::1"), 128},
      sharedNonVirtualEntry,
      sharedNonVirtualNhSet);
  EXPECT_TRUE(
      this->resourceAccountant_->checkAndUpdateEcmpResource<folly::IPAddressV6>(
          removeRoute1, false, state_));
  EXPECT_EQ(
      this->resourceAccountant_->ecmpGroupRefMap_[sharedNonVirtualNhSet], 4);
  EXPECT_EQ(this->resourceAccountant_->ecmpGroupRefMap_.size(), 7);
  EXPECT_EQ(this->resourceAccountant_->arsEcmpGroupRefMap_.size(), 3);

  // Remove one of 8 routes sharing virtual group - ref count decreases
  auto removeRoute2 = makeV6Route(
      {folly::IPAddressV6("2::1"), 128},
      sharedVirtualEntry,
      sharedVirtualNhSet);
  EXPECT_TRUE(
      this->resourceAccountant_->checkAndUpdateEcmpResource<folly::IPAddressV6>(
          removeRoute2, false, state_));
  EXPECT_EQ(this->resourceAccountant_->ecmpGroupRefMap_[sharedVirtualNhSet], 7);
  EXPECT_EQ(this->resourceAccountant_->virtualArsGroupCount_, 4);

  // Remove all remaining routes sharing virtual group
  for (int i = 1; i < 8; i++) {
    auto route = makeV6Route(
        {folly::IPAddressV6(folly::to<std::string>("2::", i + 1)), 128},
        sharedVirtualEntry,
        sharedVirtualNhSet);
    EXPECT_TRUE(this->resourceAccountant_
                    ->checkAndUpdateEcmpResource<folly::IPAddressV6>(
                        route, false, state_));
  }
  EXPECT_EQ(
      this->resourceAccountant_->ecmpGroupRefMap_.count(sharedVirtualNhSet), 0);
  EXPECT_EQ(this->resourceAccountant_->ecmpGroupRefMap_.size(), 6);
  EXPECT_EQ(this->resourceAccountant_->virtualArsGroupCount_, 3);

  // Verify virtualArsGroupCount_ > kMaxVirtualArsGroups (256) is rejected
  this->resourceAccountant_->virtualArsGroupCount_ = 257;
  EXPECT_FALSE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));

  // Verify DLB collective overhead: fill arsEcmpGroupRefMap_ to limit - 4,
  // then virtual overhead should push to exactly at limit
  this->resourceAccountant_->virtualArsGroupCount_ = 0;
  this->resourceAccountant_->arsEcmpGroupRefMap_.clear();
  for (int i = 0; i < static_cast<int>(getMaxArsGroups()) - 4; i++) {
    auto nhSet = makeNhSet(2, i + 1000);
    this->resourceAccountant_->arsEcmpGroupRefMap_[nhSet] = 1;
  }
  this->resourceAccountant_->virtualArsGroupCount_ = 10;
  // totalDlbUsage = (maxArsGroups - 4) + 4 = maxArsGroups
  EXPECT_TRUE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));

  // One more non-virtual group should exceed limit
  auto extraNhSet = makeNhSet(2, 999);
  this->resourceAccountant_->arsEcmpGroupRefMap_[extraNhSet] = 1;
  EXPECT_FALSE(this->resourceAccountant_->checkArsResource(
      true /* intermediateState */));
}

} // namespace facebook::fboss
