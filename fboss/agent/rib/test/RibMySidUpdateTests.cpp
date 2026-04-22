// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/MySidMapUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RibToSwitchStateUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/MySidMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

DECLARE_bool(enable_nexthop_id_manager);

namespace facebook::fboss {

namespace {

const RouterID kRid(0);

std::map<int64_t, cfg::SwitchInfo> getTestSwitchInfo() {
  std::map<int64_t, cfg::SwitchInfo> map;
  cfg::SwitchInfo info{};
  info.switchType() = cfg::SwitchType::NPU;
  info.asicType() = cfg::AsicType::ASIC_TYPE_FAKE;
  info.switchIndex() = 0;
  map.emplace(0, info);
  return map;
}

const SwitchIdScopeResolver* scopeResolver() {
  static const SwitchIdScopeResolver kSwitchIdScopeResolver(
      getTestSwitchInfo());
  return &kSwitchIdScopeResolver;
}

folly::CIDRNetworkV6 makeSidPrefix(const std::string& addr, uint8_t len) {
  return std::make_pair(folly::IPAddressV6(addr), len);
}

MySidEntry makeMySidEntry(
    const std::string& addr,
    uint8_t len,
    MySidType type = MySidType::DECAPSULATE_AND_LOOKUP) {
  MySidEntry entry;
  entry.type() = type;
  facebook::network::thrift::IPPrefix prefix;
  prefix.prefixAddress() =
      facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
  prefix.prefixLength() = len;
  entry.mySid() = prefix;
  return entry;
}

MySidEntry makeMySidEntryWithNextHops(
    const std::string& addr,
    uint8_t len,
    const std::vector<std::string>& nextHopAddrs) {
  MySidEntry entry = makeMySidEntry(addr, len, MySidType::NODE_MICRO_SID);
  std::vector<NextHopThrift> nextHops;
  for (const auto& nhAddr : nextHopAddrs) {
    NextHopThrift nh;
    nh.address() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(nhAddr));
    nextHops.push_back(std::move(nh));
  }
  entry.nextHops() = std::move(nextHops);
  return entry;
}

IpPrefix toIpPrefix(const std::string& addr, uint8_t len) {
  IpPrefix prefix;
  prefix.ip() = facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
  prefix.prefixLength() = len;
  return prefix;
}

// Callback that uses MySidMapUpdater to apply RIB MySid to SwitchState.
StateDelta mySidToSwitchStateUpdate(
    const SwitchIdScopeResolver* resolver,
    const NextHopIDManager* /*nextHopIDManager*/,
    const MySidTable& mySidTable,
    void* cookie) {
  auto switchState =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);
  auto oldState = *switchState;

  MySidMapUpdater updater(resolver, mySidTable);
  auto newState = updater(*switchState);
  newState->publish();
  *switchState = newState;
  return StateDelta(oldState, newState);
}

// Callback that always fails via FbossHwUpdateError.
// Uses MySidMapUpdater to build the desired state.
// Applied state is the original state (no MySid changes applied to HW).
class FailMySidUpdate {
 public:
  StateDelta operator()(
      const SwitchIdScopeResolver* resolver,
      const NextHopIDManager* /*nextHopIDManager*/,
      const MySidTable& mySidTable,
      void* cookie) {
    auto switchState =
        static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);
    (*switchState)->publish();

    // Build the desired state using MySidMapUpdater
    MySidMapUpdater updater(resolver, mySidTable);
    auto desiredState = updater(*switchState);
    desiredState->publish();

    // Applied state is the original state (no MySid changes applied)
    throw FbossHwUpdateError(desiredState, *switchState);
  }
};

// Callback that fails but injects specific MySid entries into the applied
// state, simulating a partial HW update where some MySids were programmed.
class FailWithMySidInAppliedState {
 public:
  explicit FailWithMySidInAppliedState(MySidTable mySidEntriesToInject)
      : mySidEntriesToInject_(std::move(mySidEntriesToInject)) {}

  StateDelta operator()(
      const SwitchIdScopeResolver* resolver,
      const NextHopIDManager* /*nextHopIDManager*/,
      const MySidTable& mySidTable,
      void* cookie) {
    auto switchState =
        static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);
    (*switchState)->publish();

    // Build the desired state using MySidMapUpdater
    MySidMapUpdater updater(resolver, mySidTable);
    auto desiredState = updater(*switchState);
    desiredState->publish();

    // Build the applied state from original state with injected MySids
    auto appliedState = (*switchState)->clone();
    auto newMySids = std::make_shared<MultiSwitchMySidMap>();
    for (const auto& [cidr, mySid] : mySidEntriesToInject_) {
      newMySids->addNode(
          mySid, HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}});
    }
    appliedState->resetMySids(newMySids);
    appliedState->publish();

    throw FbossHwUpdateError(desiredState, appliedState);
  }

 private:
  MySidTable mySidEntriesToInject_;
};

std::shared_ptr<MySid> makeMySid(
    const folly::CIDRNetworkV6& prefix,
    MySidType type = MySidType::DECAPSULATE_AND_LOOKUP) {
  state::MySidFields fields;
  fields.type() = type;
  facebook::network::thrift::IPPrefix thriftPrefix;
  thriftPrefix.prefixAddress() =
      facebook::network::toBinaryAddress(prefix.first);
  thriftPrefix.prefixLength() = prefix.second;
  fields.mySid() = thriftPrefix;
  auto mySid = std::make_shared<MySid>(fields);
  mySid->setUnresolveNextHopsId(std::nullopt);
  mySid->setResolvedNextHopsId(std::nullopt);
  return mySid;
}

// Callback that uses RibToSwitchStateUpdater with UPDATE_MYSID, so that
// SwitchStateNextHopIdUpdater runs and populates FibInfo id2NextHopSet maps.
StateDelta mySidToSwitchStateUpdateViaRibUpdater(
    const SwitchIdScopeResolver* resolver,
    const NextHopIDManager* nextHopIDManager,
    const MySidTable& mySidTable,
    void* cookie) {
  auto switchState =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);
  auto oldState = *switchState;

  IPv4NetworkToRouteMap emptyV4;
  IPv6NetworkToRouteMap emptyV6;
  LabelToRouteMap emptyLabel;
  RibToSwitchStateUpdater updater(
      resolver,
      RouterID(0), // don't care
      emptyV4,
      emptyV6,
      emptyLabel,
      nextHopIDManager,
      mySidTable,
      RibToSwitchStateUpdater::UPDATE_MYSID);

  auto newState = updater(*switchState);
  newState->publish();
  *switchState = newState;
  return StateDelta(oldState, newState);
}

} // namespace

TEST(RibMySidUpdate, addMySidEntry) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);

  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:100::1", 48),
  };
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  auto prefix = makeSidPrefix("fc00:100::1", 48);
  EXPECT_NE(mySidTable.find(prefix), mySidTable.end());
  EXPECT_EQ(*mySidTable[prefix].type(), MySidType::DECAPSULATE_AND_LOOKUP);

  // Verify switchState reflects the MySid
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_EQ(
      switchState->getMySids()->getNodeIf("fc00:100::1/48")->getType(),
      MySidType::DECAPSULATE_AND_LOOKUP);
}

TEST(RibMySidUpdate, addMultipleMySidEntries) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:100::1", 48),
      makeMySidEntry("fc00:200::1", 64),
  };
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add multiple mysids",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 2);
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());

  // Verify switchState reflects both MySids
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:200::1/64"), nullptr);
}

TEST(RibMySidUpdate, deleteMySidEntry) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add first
  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:100::1", 48),
      makeMySidEntry("fc00:200::1", 64),
  };
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysids",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 2);

  // Delete one
  std::vector<IpPrefix> toDelete = {toIpPrefix("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      toDelete,
      "delete mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_EQ(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());

  // Verify switchState: deleted entry gone, remaining entry present
  EXPECT_EQ(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:200::1/64"), nullptr);
}

TEST(RibMySidUpdate, deleteNonExistentMySidEntry) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add one entry
  std::vector<MySidEntry> toAdd = {makeMySidEntry("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  auto mySidTableBefore = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTableBefore.size(), 1);

  // Delete a non-existent entry — should be a no-op for that prefix
  std::vector<IpPrefix> toDelete = {toIpPrefix("fc00:999::1", 48)};
  rib.update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      toDelete,
      "delete nonexistent mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable is unchanged
  auto mySidTableAfter = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTableBefore, mySidTableAfter);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
}

TEST(RibMySidUpdate, addAndDeleteInSameUpdate) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add initial entry
  std::vector<MySidEntry> toAdd = {makeMySidEntry("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 1);

  // Add a new entry and delete the old one in the same update
  std::vector<MySidEntry> toAdd2 = {makeMySidEntry("fc00:200::1", 64)};
  std::vector<IpPrefix> toDelete = {toIpPrefix("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd2,
      toDelete,
      "add and delete mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_EQ(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());

  // Verify switchState reflects the swap
  EXPECT_EQ(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:200::1/64"), nullptr);
}

TEST(RibMySidUpdate, replaceMySidEntry) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add entry
  std::vector<MySidEntry> toAdd = {makeMySidEntry("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 1);

  // Add same prefix again (replace)
  std::vector<MySidEntry> toAdd2 = {makeMySidEntry("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd2,
      {},
      "replace mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Verify RIB mySidTable
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());

  // Verify switchState still has the entry
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
}

TEST(RibMySidUpdate, rejectNonDecapsulateType) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:100::1", 48, MySidType::NODE_MICRO_SID),
  };
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          toAdd,
          {},
          "add unsupported mysid type",
          mySidToSwitchStateUpdate,
          &switchState),
      FbossError);

  // Verify RIB and switchState remain empty
  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}

TEST(RibMySidUpdate, rejectEntryWithNextHops) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  auto entry = makeMySidEntry("fc00:100::1", 48);
  NextHopThrift nhop;
  nhop.address() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2::2"));
  entry.nextHops() = {nhop};

  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          {entry},
          {},
          "add mysid with nexthops",
          mySidToSwitchStateUpdate,
          &switchState),
      FbossError);

  // Verify RIB and switchState remain empty
  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}

TEST(RibMySidUpdate, rejectBothNextHopsAndNamedNextHops) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  auto entry = makeMySidEntry("fc00:100::1", 48, MySidType::NODE_MICRO_SID);
  NextHopThrift nhop;
  nhop.address() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2::2"));
  entry.nextHops() = {nhop};
  NamedRouteDestination named;
  named.nextHopGroups() = {"group1"};
  entry.namedNextHops() = named;

  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          {entry},
          {},
          "add mysid with both nexthops and named nexthops",
          mySidToSwitchStateUpdate,
          &switchState),
      FbossError);

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}

TEST(RibMySidUpdate, rejectDecapsulateTypeWithNamedNextHops) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  auto entry = makeMySidEntry("fc00:100::1", 48);
  NamedRouteDestination named;
  named.nextHopGroups() = {"group1"};
  entry.namedNextHops() = named;

  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          {entry},
          {},
          "add decap mysid with named nexthops",
          mySidToSwitchStateUpdate,
          &switchState),
      FbossError);

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}

TEST(RibMySidUpdate, invalidEntryInBatchDoesNotPartiallyMutateMySidTable) {
  // Verify that if any entry in toAdd is invalid, mySidFromEntry throws before
  // updateRibMySids is called, leaving mySidTable completely unmodified.
  // Without the pre-build fix, the valid entry (first in the vector) would be
  // written to mySidTable before the invalid entry throws. With the fix,
  // neither entry is written.
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add an initial valid entry so the table is non-empty going in
  rib.update(
      scopeResolver(),
      {makeMySidEntry("fc00:100::1", 48)},
      {},
      "add initial mysid",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 1);

  // Batch: valid entry first, then invalid (NODE_MICRO_SID with no nexthops)
  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:200::1", 64), // valid
      makeMySidEntry(
          "fc00:300::1", 48, MySidType::NODE_MICRO_SID), // invalid: no nexthops
  };
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          toAdd,
          {},
          "batch with invalid entry",
          mySidToSwitchStateUpdate,
          &switchState),
      FbossError);

  // mySidTable must contain only the original entry — the valid entry from
  // the failed batch must not have been inserted
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_EQ(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());
}

TEST(RibMySidUpdate, switchStateUpdatedOnAdd) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();
  auto origState = switchState;

  std::vector<MySidEntry> toAdd = {makeMySidEntry("fc00:100::1", 48)};
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Switch state should have changed
  EXPECT_NE(switchState, origState);
  // MySid should be in switch state with correct type
  auto mySidNode = switchState->getMySids()->getNodeIf("fc00:100::1/48");
  ASSERT_NE(mySidNode, nullptr);
  EXPECT_EQ(mySidNode->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
}

TEST(RibMySidUpdate, switchStateUpdatedOnDelete) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Add two entries
  std::vector<MySidEntry> toAdd = {
      makeMySidEntry("fc00:100::1", 48),
      makeMySidEntry("fc00:200::1", 64),
  };
  rib.update(
      scopeResolver(),
      toAdd,
      {},
      "add mysids",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(switchState->getMySids()->numNodes(), 2);

  auto stateBeforeDelete = switchState;

  // Delete one
  rib.update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      {toIpPrefix("fc00:100::1", 48)},
      "delete mysid",
      mySidToSwitchStateUpdate,
      &switchState);

  // Switch state should have changed
  EXPECT_NE(switchState, stateBeforeDelete);
  // Only one MySid should remain in switch state
  EXPECT_EQ(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:200::1/64"), nullptr);
}

TEST(RibMySidUpdate, rollbackOnFailedUpdate) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);

  // Try to add a MySid entry but fail the HW update
  std::vector<MySidEntry> toAdd = {makeMySidEntry("fc00:100::1", 48)};
  FailMySidUpdate failUpdate;
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          toAdd,
          {},
          "fail mysid update",
          failUpdate,
          &switchState),
      FbossHwUpdateError);

  // After rollback, mySidTable should be reconstructed from applied state
  // (which is the original empty state), so it should remain empty
  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
}

TEST(RibMySidUpdate, rollbackPreservesAppliedStateMySids) {
  // Verify that after a failed update, the RIB mySidTable is reconstructed
  // from the applied state's MySidMap (which may contain MySid entries
  // that were partially programmed to HW).
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);

  // Inject a MySid entry into the applied state during failure
  auto injectedPrefix = makeSidPrefix("fc00:999::1", 48);
  MySidTable injectedMySids;
  injectedMySids[injectedPrefix] = makeMySid(injectedPrefix);

  FailWithMySidInAppliedState failWithInjected(injectedMySids);
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          {makeMySidEntry("fc00:100::1", 48)},
          {},
          "fail with injected mysid",
          failWithInjected,
          &switchState),
      FbossHwUpdateError);

  // RIB mySidTable should be reconstructed from applied state,
  // which has the injected entry (not the entry we tried to add)
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_NE(mySidTable.find(injectedPrefix), mySidTable.end());
  EXPECT_EQ(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
}

TEST(RibMySidUpdate, rollbackWithPreExistingEntries) {
  // Verify rollback when the RIB already has MySid entries.
  // After a failed add, the RIB should reflect the applied state's MySids,
  // not the pre-update RIB state.
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Successfully add an initial MySid entry
  rib.update(
      scopeResolver(),
      {makeMySidEntry("fc00:100::1", 48)},
      {},
      "add initial mysid",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 1);
  EXPECT_NE(switchState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);

  // Now fail an update that tries to add a second entry.
  // The applied state contains the original entry (fc00:100::1/48).
  FailMySidUpdate failUpdate;
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          {makeMySidEntry("fc00:200::1", 64)},
          {},
          "fail second add",
          failUpdate,
          &switchState),
      FbossHwUpdateError);

  // RIB mySidTable should be reconstructed from applied state,
  // which still has only the original entry
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_EQ(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());
}

TEST(RibMySidUpdate, rollbackDeleteRestoresEntries) {
  // Verify rollback when a delete fails.
  // The applied state still has the entry we tried to delete.
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Successfully add two MySid entries
  rib.update(
      scopeResolver(),
      {makeMySidEntry("fc00:100::1", 48), makeMySidEntry("fc00:200::1", 64)},
      {},
      "add mysids",
      mySidToSwitchStateUpdate,
      &switchState);
  EXPECT_EQ(rib.getMySidTableCopy().size(), 2);

  // Fail an update that tries to delete one entry.
  // The applied state still has both entries.
  FailMySidUpdate failUpdate;
  EXPECT_THROW(
      rib.update(
          scopeResolver(),
          std::vector<MySidEntry>{},
          {toIpPrefix("fc00:100::1", 48)},
          "fail delete",
          failUpdate,
          &switchState),
      FbossHwUpdateError);

  // RIB mySidTable should be reconstructed from applied state,
  // which still has both entries (delete was not applied to HW)
  auto mySidTable = rib.getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 2);
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:100::1", 48)), mySidTable.end());
  EXPECT_NE(
      mySidTable.find(makeSidPrefix("fc00:200::1", 64)), mySidTable.end());
}

TEST(RibMySidUpdate, updaterClonesPublishedState) {
  // Verify that MySidMapUpdater clones the state before mutating it.
  // A previous bug had MySidMapUpdater returning a shared_ptr copy (not a
  // clone), which mutated the already-published input state and caused an
  // assertion failure.
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();

  // Build a RIB MySidTable with one entry
  MySidTable mySidTable;
  auto prefix = makeSidPrefix("fc00:100::1", 48);
  mySidTable[prefix] = makeMySid(prefix);

  MySidMapUpdater updater(scopeResolver(), mySidTable);
  auto newState = updater(switchState);

  // Returned state must be a different object (cloned, not aliased)
  EXPECT_NE(newState.get(), switchState.get());
  // Original published state must not have been mutated
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
  // New state should have the entry
  EXPECT_NE(newState->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
}

TEST(RibMySidUpdate, emptyUpdate) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();
  auto origState = switchState;

  // Empty update should be a no-op
  rib.update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      std::vector<IpPrefix>{},
      "empty mysid update",
      mySidToSwitchStateUpdate,
      &switchState);

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  // switchState should not change on empty update
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}

class RibMySidNextHopTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    rib_ = std::make_unique<RoutingInformationBase>();
    switchState_ = std::make_shared<SwitchState>();
    switchState_->publish();
    rib_->ensureVrf(kRid);
  }

  std::unique_ptr<RoutingInformationBase> rib_;
  std::shared_ptr<SwitchState> switchState_;
};

TEST_F(RibMySidNextHopTest, newEntryWithNextHopsAllocatesNextHopSetId) {
  // Adding a MySid entry with next hops should allocate a NextHopSetID.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto mySidTable = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  ASSERT_NE(mySidTable.find(prefix), mySidTable.end());
  EXPECT_TRUE(mySidTable.at(prefix).unresolveNextHopsId().has_value());
}

TEST_F(RibMySidNextHopTest, replaceMySidWithSameNextHopsReusesNextHopSetId) {
  // Replacing a MySid entry with identical next hops should reuse the same ID.
  const std::vector<std::string> nextHops = {"2001:db8::1"};
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, nextHops)},
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterFirst = rib_->getMySidTableCopy();
  const auto idAfterFirstAdd =
      tableAfterFirst.at(makeSidPrefix("fc00:100::1", 48))
          .unresolveNextHopsId();
  ASSERT_TRUE(idAfterFirstAdd.has_value());

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, nextHops)},
      {},
      "re-add mysid same nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterSecond = rib_->getMySidTableCopy();
  const auto idAfterSecondAdd =
      tableAfterSecond.at(makeSidPrefix("fc00:100::1", 48))
          .unresolveNextHopsId();
  EXPECT_EQ(idAfterFirstAdd, idAfterSecondAdd);
}

TEST_F(
    RibMySidNextHopTest,
    replaceMySidWithDifferentNextHopsUpdatesNextHopSetId) {
  // Replacing a MySid entry with different next hops should allocate a new ID.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterFirst = rib_->getMySidTableCopy();
  const auto idAfterFirstAdd =
      tableAfterFirst.at(makeSidPrefix("fc00:100::1", 48))
          .unresolveNextHopsId();
  ASSERT_TRUE(idAfterFirstAdd.has_value());

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops(
          "fc00:100::1", 48, {"2001:db8::3", "2001:db8::4"})},
      {},
      "re-add mysid different nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterSecond = rib_->getMySidTableCopy();
  const auto idAfterSecondAdd =
      tableAfterSecond.at(makeSidPrefix("fc00:100::1", 48))
          .unresolveNextHopsId();
  ASSERT_TRUE(idAfterSecondAdd.has_value());
  EXPECT_NE(idAfterFirstAdd, idAfterSecondAdd);
}

TEST_F(RibMySidNextHopTest, replaceMySidWithNoNextHopsClearsNextHopSetId) {
  // Replacing a MySid entry with an entry that has no next hops should clear
  // unresolveNextHopsId and release the old ID without crash.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  ASSERT_TRUE(
      rib_->getMySidTableCopy().at(prefix).unresolveNextHopsId().has_value());

  rib_->update(
      scopeResolver(),
      {makeMySidEntry("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP)},
      {},
      "replace mysid with no nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  EXPECT_FALSE(
      rib_->getMySidTableCopy().at(prefix).unresolveNextHopsId().has_value());
}

TEST_F(RibMySidNextHopTest, deleteMySidWithNextHopsReleasesNextHopSetId) {
  // Deleting a MySid entry that has next hops should release the ID.
  // Verified by re-adding the same entry after deletion — no crash expected.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  ASSERT_TRUE(
      rib_->getMySidTableCopy().at(prefix).unresolveNextHopsId().has_value());

  rib_->update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      {toIpPrefix("fc00:100::1", 48)},
      "delete mysid",
      mySidToSwitchStateUpdate,
      &switchState_);

  EXPECT_EQ(rib_->getMySidTableCopy().count(prefix), 0);

  // Re-add after delete should succeed and allocate a fresh ID.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "re-add mysid after delete",
      mySidToSwitchStateUpdate,
      &switchState_);

  EXPECT_TRUE(
      rib_->getMySidTableCopy().at(prefix).unresolveNextHopsId().has_value());
}

TEST_F(
    RibMySidNextHopTest,
    updateFibFailureRestoresMySidTableAndNextHopIdManager) {
  // Add entry A with nexthops — succeeds, allocates ID_A in the manager
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid A",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefixA = makeSidPrefix("fc00:100::1", 48);

  // Fail adding entry B — FailMySidUpdate uses the current switchState_ (with
  // A) as the applied state, so rollback should restore to A-only state
  FailMySidUpdate failUpdate;
  EXPECT_THROW(
      rib_->update(
          scopeResolver(),
          {makeMySidEntryWithNextHops("fc00:200::1", 64, {"2001:db8::2"})},
          {},
          "fail adding mysid B",
          failUpdate,
          &switchState_),
      FbossHwUpdateError);

  // mySidTable should be restored to only contain A
  const auto mySidTable = rib_->getMySidTableCopy();
  EXPECT_EQ(mySidTable.size(), 1);
  EXPECT_NE(mySidTable.find(prefixA), mySidTable.end());

  // A subsequent successful add of B confirms the manager fully recovered
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:200::1", 64, {"2001:db8::2"})},
      {},
      "add mysid B after recovery",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefixB = makeSidPrefix("fc00:200::1", 64);
  const auto tableAfter = rib_->getMySidTableCopy();
  EXPECT_EQ(tableAfter.size(), 2);
  const auto idBOpt = tableAfter.at(prefixB).unresolveNextHopsId();
  EXPECT_TRUE(idBOpt.has_value());
}

TEST_F(
    RibMySidNextHopTest,
    addMySidWithGatewayNextHop_noRoutes_resolvedSetIdNotSet) {
  // Gateway nexthop (no interface ID) with no matching VRF 0 route → resolve
  // finds nothing, so resolvedNextHopsId should remain unset.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with unresolvable gateway nexthop",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  EXPECT_FALSE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());
}

TEST_F(
    RibMySidNextHopTest,
    addMySidWithGatewayNextHop_withMatchingRoute_resolvedSetIdSet) {
  // Inject a resolved interface route covering the gateway nexthop's address
  // via reconfigure, then add the MySid entry → resolvedNextHopsId is set.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 32}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::1")};
  rib_->reconfigure(
      scopeResolver(),
      interfaceRoutes,
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      noopFibUpdate,
      &switchState_);

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with resolvable gateway nexthop",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  EXPECT_TRUE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());
}

// Replacing next hops on a MySid entry should:
// 1. Deallocate the old NextHopSetID (getNextHopsIf returns nullopt)
// 2. Keep the new NextHopSetID alive (getNextHopsIf returns non-null)
TEST_F(
    RibMySidNextHopTest,
    replaceNextHopsDecrementsOldAndIncrementsNewRefCount) {
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops(
          "fc00:100::1", 48, {"2001:db8::1", "2001:db8::2"})},
      {},
      "add mysid with nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterFirst = rib_->getMySidTableCopy();
  const auto oldIdOpt = tableAfterFirst.at(makeSidPrefix("fc00:100::1", 48))
                            .unresolveNextHopsId();
  ASSERT_TRUE(oldIdOpt.has_value());
  const auto oldId = NextHopSetID(*oldIdOpt);

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops(
          "fc00:100::1", 48, {"2001:db8::3", "2001:db8::4"})},
      {},
      "replace mysid with different nexthops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto tableAfterSecond = rib_->getMySidTableCopy();
  const auto newIdOpt = tableAfterSecond.at(makeSidPrefix("fc00:100::1", 48))
                            .unresolveNextHopsId();
  ASSERT_TRUE(newIdOpt.has_value());
  const auto newId = NextHopSetID(*newIdOpt);

  auto manager = rib_->getNextHopIDManagerCopy();
  ASSERT_NE(manager, nullptr);
  // Old next hop set should have been deallocated (refcount dropped to 0)
  EXPECT_FALSE(manager->getNextHopsIf(oldId).has_value());
  // New next hop set should still be allocated
  EXPECT_TRUE(manager->getNextHopsIf(newId).has_value());
}

// Replacing a uN MySid (with both unresolved and resolved next-hop IDs)
// with a decap MySid (no next hops) must decrement the manager's
// reference for BOTH the old unresolveNextHopsId and the old
// resolvedNextHopsId.
//
// The resolvedId for a route-resolved gateway is shared with the matching
// route's own next-hop set, so the SetID entry stays alive after the MySid
// release (held by the route). We pin the decrement contract via the
// per-NextHop refcount: each MySid set contributes +1 to each NextHop in
// its set, so after replacing the MySid the resolved NextHop's refcount
// must drop by 1. Without the fix the MySid's reference is never
// released, the NextHop refcount stays flat — that's the leak.
TEST_F(
    RibMySidNextHopTest,
    replaceNodeMySidWithDecapReleasesBothUnresolvedAndResolvedIds) {
  // Inject an interface route that the gateway nexthop will resolve via.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 32}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::1")};
  rib_->reconfigure(
      scopeResolver(),
      interfaceRoutes,
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      noopFibUpdate,
      &switchState_);

  // Add a NODE_MICRO_SID with a gateway next hop that resolves through the
  // interface route — this populates BOTH unresolveNextHopsId and
  // resolvedNextHopsId on the entry.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add uN mysid with resolvable gateway nexthop",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  const auto entryBefore = rib_->getMySidTableCopy().at(prefix);
  ASSERT_TRUE(entryBefore.unresolveNextHopsId().has_value());
  ASSERT_TRUE(entryBefore.resolvedNextHopsId().has_value())
      << "Test setup: NODE_MICRO_SID gateway should have resolved through "
         "the interface route";

  // Capture per-NextHop refcounts before the replace. The interface route
  // contributes 1 ref to the resolved nexthop; the MySid's resolved set
  // contributes a second ref. The unresolved gateway nexthop is held only
  // by the MySid's unresolved set.
  const ResolvedNextHop resolvedNextHop{
      folly::IPAddress("2001:db8::1"), InterfaceID(1), NextHopWeight(1)};
  const UnresolvedNextHop unresolvedNextHop{
      folly::IPAddress("2001:db8::1"), ECMP_WEIGHT};
  uint32_t resolvedRefBefore = 0;
  uint32_t unresolvedRefBefore = 0;
  {
    auto manager = rib_->getNextHopIDManagerCopy();
    ASSERT_NE(manager, nullptr);
    resolvedRefBefore = manager->getNextHopRefCount(resolvedNextHop);
    unresolvedRefBefore = manager->getNextHopRefCount(unresolvedNextHop);
    EXPECT_GE(resolvedRefBefore, 2u)
        << "Test setup: resolved NextHop should be shared by route + MySid";
    EXPECT_GE(unresolvedRefBefore, 1u)
        << "Test setup: unresolved gateway NextHop should be held by MySid";
  }

  // Replace with a DECAP entry (no next hops at all). The fix releases
  // both of the old MySid's NextHop set references.
  rib_->update(
      scopeResolver(),
      {makeMySidEntry("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP)},
      {},
      "replace with decap mysid",
      mySidToSwitchStateUpdate,
      &switchState_);

  // The new entry should have neither id set.
  const auto entryAfter = rib_->getMySidTableCopy().at(prefix);
  EXPECT_FALSE(entryAfter.unresolveNextHopsId().has_value());
  EXPECT_FALSE(entryAfter.resolvedNextHopsId().has_value());

  // Both NextHop refcounts must have decreased by exactly one. Without
  // the fix, resolvedRefAfter would equal resolvedRefBefore — the leak.
  auto manager = rib_->getNextHopIDManagerCopy();
  ASSERT_NE(manager, nullptr);
  EXPECT_EQ(manager->getNextHopRefCount(resolvedNextHop), resolvedRefBefore - 1)
      << "Resolved NextHop refcount didn't drop on MySid replace — "
         "resolvedNextHopsId leak persists";
  EXPECT_EQ(
      manager->getNextHopRefCount(unresolvedNextHop), unresolvedRefBefore - 1)
      << "Unresolved NextHop refcount didn't drop on MySid replace";
}

// Test fixture that sets up a SwitchState with a FibInfo node so that
// SwitchStateNextHopIdUpdater (called via RibToSwitchStateUpdater with
// UPDATE_MYSID) can populate FibInfo's id2NextHopSet maps.
class RibMySidFibInfoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    rib_ = std::make_unique<RoutingInformationBase>();
    switchState_ = std::make_shared<SwitchState>();
    // Add a FibInfo node so SwitchStateNextHopIdUpdater has a node to write to.
    auto fibsInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
    fibsInfoMap->addNode("id=0", std::make_shared<FibInfo>());
    switchState_->resetFibsInfoMap(fibsInfoMap);
    switchState_->publish();
    rib_->ensureVrf(kRid);
  }

  // Returns the IdToNextHopIdSetMap from the FibInfo node in the SwitchState.
  std::shared_ptr<IdToNextHopIdSetMap> getIdToNextHopIdSetMap() {
    auto fibInfo = switchState_->getFibsInfoMap()->getNodeIf("id=0");
    return fibInfo ? fibInfo->getIdToNextHopIdSetMap() : nullptr;
  }

  std::unique_ptr<RoutingInformationBase> rib_;
  std::shared_ptr<SwitchState> switchState_;
};

TEST_F(RibMySidFibInfoTest, mySidNextHopSetIdReflectedInFibInfo) {
  // Adding a MySid entry with next hops allocates a NextHopSetID.  Verify that
  // the allocated ID is written into FibInfo->id2NextHopIdSet by
  // SwitchStateNextHopIdUpdater (called through RibToSwitchStateUpdater with
  // UPDATE_MYSID).
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with nexthops",
      mySidToSwitchStateUpdateViaRibUpdater,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  const auto mySidTable = rib_->getMySidTableCopy();
  ASSERT_NE(mySidTable.find(prefix), mySidTable.end());
  const auto unresolvedId = mySidTable.at(prefix).unresolveNextHopsId();
  ASSERT_TRUE(unresolvedId.has_value());

  // The allocated NextHopSetID must appear in FibInfo's id2NextHopIdSet.
  auto idSetMap = getIdToNextHopIdSetMap();
  ASSERT_NE(idSetMap, nullptr);
  EXPECT_NE(idSetMap->getNextHopIdSetIf(*unresolvedId), nullptr)
      << "MySid unresolvedNextHopsId not found in FibInfo id2NextHopIdSet";
}

TEST_F(RibMySidFibInfoTest, deleteMySidClearsFibInfoNextHopSetId) {
  // After deleting a MySid entry, its NextHopSetID should be removed from
  // FibInfo->id2NextHopIdSet.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid",
      mySidToSwitchStateUpdateViaRibUpdater,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  const auto tableAfterAdd = rib_->getMySidTableCopy();
  ASSERT_NE(tableAfterAdd.find(prefix), tableAfterAdd.end());
  const auto idAfterAdd = tableAfterAdd.at(prefix).unresolveNextHopsId();
  ASSERT_TRUE(idAfterAdd.has_value());

  rib_->update(
      scopeResolver(),
      std::vector<MySidEntry>{},
      {toIpPrefix("fc00:100::1", 48)},
      "delete mysid",
      mySidToSwitchStateUpdateViaRibUpdater,
      &switchState_);

  // After deletion the NextHopSetID must be absent from FibInfo.
  auto idSetMap = getIdToNextHopIdSetMap();
  ASSERT_NE(idSetMap, nullptr);
  EXPECT_EQ(idSetMap->getNextHopIdSetIf(*idAfterAdd), nullptr)
      << "MySid NextHopSetId should have been removed from FibInfo after delete";
}

TEST_F(RibMySidFibInfoTest, mySidOnlyNextHopSetIdsInFibInfo) {
  // With no routes present, only MySid-allocated NextHopSetIDs should appear
  // in FibInfo id2NextHopIdSet (i.e. they are not shared with any route).
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops(
          "fc00:100::1", 48, {"2001:db8::1", "2001:db8::2"})},
      {},
      "add mysid with two nexthops",
      mySidToSwitchStateUpdateViaRibUpdater,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  const auto mySidTable = rib_->getMySidTableCopy();
  ASSERT_NE(mySidTable.find(prefix), mySidTable.end());
  const auto unresolvedId = mySidTable.at(prefix).unresolveNextHopsId();
  ASSERT_TRUE(unresolvedId.has_value());

  // FibInfo should have exactly one NextHopSetID entry — the MySid's unresolved
  // set — confirming it is not shared with any route.
  auto idSetMap = getIdToNextHopIdSetMap();
  ASSERT_NE(idSetMap, nullptr);
  EXPECT_EQ(idSetMap->size(), 1u);
  EXPECT_NE(idSetMap->getNextHopIdSetIf(*unresolvedId), nullptr);
}

TEST_F(RibMySidFibInfoTest, resolvedNextHopSetIdReflectedInFibInfo) {
  // When a MySid entry's gateway nexthop can be resolved via an interface
  // route, both unresolvedNextHopsId and resolvedNextHopsId should appear in
  // FibInfo->id2NextHopIdSet.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 32}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::1")};
  rib_->reconfigure(
      scopeResolver(),
      interfaceRoutes,
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      noopFibUpdate,
      &switchState_);

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with resolvable gateway nexthop",
      mySidToSwitchStateUpdateViaRibUpdater,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  const auto mySidTable = rib_->getMySidTableCopy();
  ASSERT_NE(mySidTable.find(prefix), mySidTable.end());
  const auto unresolvedId = mySidTable.at(prefix).unresolveNextHopsId();
  const auto resolvedId = mySidTable.at(prefix).resolvedNextHopsId();
  ASSERT_TRUE(unresolvedId.has_value());
  ASSERT_TRUE(resolvedId.has_value());

  auto idSetMap = getIdToNextHopIdSetMap();
  ASSERT_NE(idSetMap, nullptr);
  EXPECT_NE(idSetMap->getNextHopIdSetIf(*unresolvedId), nullptr)
      << "MySid unresolvedNextHopsId not found in FibInfo id2NextHopIdSet";
  EXPECT_NE(idSetMap->getNextHopIdSetIf(*resolvedId), nullptr)
      << "MySid resolvedNextHopsId not found in FibInfo id2NextHopIdSet";
}

TEST_F(RibMySidNextHopTest, routeUpdateResolvesMySidNextHops) {
  // Add a MySid entry whose nexthop can't be resolved yet (no route exists).
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with unresolvable nexthop",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  ASSERT_TRUE(
      rib_->getMySidTableCopy().at(prefix).unresolveNextHopsId().has_value());
  EXPECT_FALSE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());

  // Add an interface route covering the MySid nexthop address.
  // This route update must trigger re-resolution of MySid entries.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 64}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::2")};
  rib_->updateRemoteInterfaceRoutes(
      scopeResolver(), interfaceRoutes, {}, noopFibUpdate, &switchState_);

  EXPECT_TRUE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());
}

TEST_F(RibMySidNextHopTest, routeDeleteUnresolvesMySidNextHops) {
  // Add an interface route and a MySid whose nexthop resolves through it.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 64}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::2")};
  rib_->updateRemoteInterfaceRoutes(
      scopeResolver(), interfaceRoutes, {}, noopFibUpdate, &switchState_);

  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("fc00:100::1", 48, {"2001:db8::1"})},
      {},
      "add mysid with resolvable nexthop",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("fc00:100::1", 48);
  ASSERT_TRUE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());

  // Delete the interface route. MySid nexthop should become unresolved.
  boost::container::flat_map<RouterID, std::vector<folly::CIDRNetwork>> toDel;
  toDel[kRid].push_back({folly::IPAddress("2001:db8::"), 64});
  rib_->updateRemoteInterfaceRoutes(
      scopeResolver(), {}, toDel, noopFibUpdate, &switchState_);

  EXPECT_FALSE(
      rib_->getMySidTableCopy().at(prefix).resolvedNextHopsId().has_value());
}

} // namespace facebook::fboss
