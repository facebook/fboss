// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/MySidConfigUtils.h"
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

// Locator with nonzero second 16-bit group so the resulting SID address
// has an unambiguous IPv6 string form (e.g., "3001:db8:100::" rather
// than the IPv6-compacted "fc00:0:100::" for an all-zero second group).
constexpr folly::StringPiece kTestLocatorPrefix = "3001:db8::/32";

cfg::MySidConfig makeDecapMySidConfig(int16_t functionId) {
  cfg::MySidConfig mySidConfig;
  mySidConfig.locatorPrefix() = kTestLocatorPrefix.str();
  cfg::MySidEntryConfig entry;
  entry.set_decap(cfg::DecapMySidConfig{});
  mySidConfig.entries()->emplace(functionId, std::move(entry));
  return mySidConfig;
}

cfg::MySidConfig makeNodeMySidConfig(
    int16_t functionId,
    const std::string& nodeAddress) {
  cfg::MySidConfig mySidConfig;
  mySidConfig.locatorPrefix() = kTestLocatorPrefix.str();
  cfg::NodeMySidConfig node;
  node.nodeAddress() = nodeAddress;
  cfg::MySidEntryConfig entry;
  entry.set_node(std::move(node));
  mySidConfig.entries()->emplace(functionId, std::move(entry));
  return mySidConfig;
}

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

  // Materialize the unresolveNextHopsId for `prefix` into a value to dodge
  // the thrift `field_ref` lifetime trap: getMySidTableCopy() returns a
  // temporary, and the field_ref returned by unresolveNextHopsId() points
  // into it.
  std::optional<int64_t> getUnresolveId(
      const folly::CIDRNetworkV6& prefix) const {
    const auto table = rib_->getMySidTableCopy();
    auto it = table.find(prefix);
    if (it == table.end()) {
      return std::nullopt;
    }
    if (auto v = it->second.unresolveNextHopsId()) {
      return *v;
    }
    return std::nullopt;
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
      {} /* staticMySids */,
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
      {} /* staticMySids */,
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
      {} /* staticMySids */,
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

// Tests for MySid programming via reconfigure() — ensures config-driven
// MySid entries passed through reconfigure reach the RIB mySidTable.

TEST_F(RibMySidNextHopTest, reconfigureAddsMySidToRibTable) {
  auto mySidConfig = makeDecapMySidConfig(0x100);

  rib_->reconfigure(
      scopeResolver(),
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_EQ(table.at(prefix).type().value(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_EQ(table.at(prefix).clientId().value(), ClientID::STATIC_ROUTE);
}

TEST_F(RibMySidNextHopTest, reconfigureDeletesMySidFromRibTable) {
  // Keep kRid alive across both reconfigure calls — production callers
  // always pass at least one VRF in configRouterIDToInterfaceRoutes.
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  auto mySidConfig = makeDecapMySidConfig(0x100);
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
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);
  {
    const auto tableAfterAdd = rib_->getMySidTableCopy();
    ASSERT_NE(
        tableAfterAdd.find(makeSidPrefix("3001:db8:100::", 48)),
        tableAfterAdd.end());
  }

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
      {} /* staticMySids */,
      noopFibUpdate,
      &switchState_);

  const auto tableAfterDelete = rib_->getMySidTableCopy();
  EXPECT_EQ(
      tableAfterDelete.find(makeSidPrefix("3001:db8:100::", 48)),
      tableAfterDelete.end());
}

TEST_F(RibMySidNextHopTest, reconfigureMySidWithNextHopsAllocatesId) {
  auto mySidConfig = makeNodeMySidConfig(0x100, "2001:db8::1");

  rib_->reconfigure(
      scopeResolver(),
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_TRUE(table.at(prefix).unresolveNextHopsId().has_value());
}

TEST_F(RibMySidNextHopTest, reconfigureMySidAndRoutesTogetherBothInstalled) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("10.0.0.0"), 24}] = {
      InterfaceID(1), folly::IPAddress("10.0.0.1")};

  auto mySidConfig = makeDecapMySidConfig(0x100);

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
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_EQ(table.at(prefix).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_NE(
      rib_->longestMatch<folly::IPAddressV4>(
          folly::IPAddressV4("10.0.0.1"), kRid),
      nullptr);
}

TEST_F(RibMySidNextHopTest, reconfigureNodeMySidWithRouteResolved) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid][{folly::IPAddress("2001:db8::"), 32}] = {
      InterfaceID(1), folly::IPAddress("2001:db8::1")};

  auto mySidConfig = makeNodeMySidConfig(0x100, "2001:db8::1");

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
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_TRUE(table.at(prefix).unresolveNextHopsId().has_value());
  // Route is present, so MySid should be resolved via LPM
  EXPECT_TRUE(table.at(prefix).resolvedNextHopsId().has_value());
}

// Non-STATIC_ROUTE MySid entries (default ClientID = TE_AGENT) must not be
// removed by a reconfigure() that supplies an empty mySidConfig.
TEST_F(RibMySidNextHopTest, reconfigurePreservesTeAgentMySid) {
  // Add an entry via the RPC path (clientId defaults to TE_AGENT).
  rib_->update(
      scopeResolver(),
      {makeMySidEntry("fc00:200::1", 48)},
      {},
      "add te_agent mysid",
      mySidToSwitchStateUpdate,
      &switchState_);
  ASSERT_NE(
      rib_->getMySidTableCopy().find(makeSidPrefix("fc00:200::1", 48)),
      rib_->getMySidTableCopy().end());

  // reconfigure with no mySidConfig should clear STATIC_ROUTE only — leave
  // the TE_AGENT entry intact.
  rib_->reconfigure(
      scopeResolver(),
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      {},
      {} /* staticMySids */,
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  EXPECT_NE(table.find(makeSidPrefix("fc00:200::1", 48)), table.end());
}

// Reconfigure with the same prefix but a different MySid type — second
// reconfigure must overwrite the first entry, not coexist or skip.
TEST_F(RibMySidNextHopTest, reconfigureUpdatesMySidType) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  // First reconfigure: install DECAP entry.
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
      convertMySidConfig(makeDecapMySidConfig(0x100), {}),
      noopFibUpdate,
      &switchState_);
  {
    const auto table = rib_->getMySidTableCopy();
    const auto prefix = makeSidPrefix("3001:db8:100::", 48);
    ASSERT_NE(table.find(prefix), table.end());
    EXPECT_EQ(
        table.at(prefix).type().value(), MySidType::DECAPSULATE_AND_LOOKUP);
    EXPECT_FALSE(table.at(prefix).unresolveNextHopsId().has_value());
  }

  // Second reconfigure: same prefix (functionId 0x100) but NODE type with
  // a next hop. Must replace the DECAP entry and allocate a new nhopId.
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
      convertMySidConfig(makeNodeMySidConfig(0x100, "2001:db8::1"), {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_EQ(table.at(prefix).type().value(), MySidType::NODE_MICRO_SID);
  EXPECT_EQ(table.at(prefix).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_TRUE(table.at(prefix).unresolveNextHopsId().has_value());
}

// Reconfigure with multiple STATIC entries, then with a subset — only the
// removed prefix is gone; the kept prefix survives.
TEST_F(RibMySidNextHopTest, reconfigurePartiallyDeletesMySid) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  // Install two DECAP entries.
  cfg::MySidConfig twoEntries;
  twoEntries.locatorPrefix() = kTestLocatorPrefix.str();
  cfg::MySidEntryConfig decap;
  decap.set_decap(cfg::DecapMySidConfig{});
  twoEntries.entries()->emplace(0x100, decap);
  twoEntries.entries()->emplace(0x200, decap);

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
      convertMySidConfig(twoEntries, {}),
      noopFibUpdate,
      &switchState_);
  {
    const auto table = rib_->getMySidTableCopy();
    EXPECT_NE(table.find(makeSidPrefix("3001:db8:100::", 48)), table.end());
    EXPECT_NE(table.find(makeSidPrefix("3001:db8:200::", 48)), table.end());
  }

  // Reconfigure with only one of the two entries — the dropped one must
  // disappear, the kept one must remain.
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
      convertMySidConfig(makeDecapMySidConfig(0x100), {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:100::", 48)), table.end());
  EXPECT_EQ(table.find(makeSidPrefix("3001:db8:200::", 48)), table.end());
}

// Reconfigure with multiple MySid entries of mixed types in a single call —
// every entry should land with the correct type and clientId.
TEST_F(RibMySidNextHopTest, reconfigureMultipleMySidEntries) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  cfg::MySidConfig multiConfig;
  multiConfig.locatorPrefix() = kTestLocatorPrefix.str();
  cfg::MySidEntryConfig decap;
  decap.set_decap(cfg::DecapMySidConfig{});
  multiConfig.entries()->emplace(0x100, decap);
  multiConfig.entries()->emplace(0x200, decap);

  cfg::NodeMySidConfig node;
  node.nodeAddress() = "2001:db8::1";
  cfg::MySidEntryConfig nodeEntry;
  nodeEntry.set_node(std::move(node));
  multiConfig.entries()->emplace(0x300, std::move(nodeEntry));

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
      convertMySidConfig(multiConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto p100 = makeSidPrefix("3001:db8:100::", 48);
  const auto p200 = makeSidPrefix("3001:db8:200::", 48);
  const auto p300 = makeSidPrefix("3001:db8:300::", 48);
  ASSERT_NE(table.find(p100), table.end());
  ASSERT_NE(table.find(p200), table.end());
  ASSERT_NE(table.find(p300), table.end());
  EXPECT_EQ(table.at(p100).type().value(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_EQ(table.at(p200).type().value(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_EQ(table.at(p300).type().value(), MySidType::NODE_MICRO_SID);
  EXPECT_EQ(table.at(p100).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_EQ(table.at(p200).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_EQ(table.at(p300).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_FALSE(table.at(p100).unresolveNextHopsId().has_value());
  EXPECT_FALSE(table.at(p200).unresolveNextHopsId().has_value());
  EXPECT_TRUE(table.at(p300).unresolveNextHopsId().has_value());
}

// Re-applying an identical mySidConfig must NOT churn next-hop IDs: the
// existing entry should be left in place (same unresolveNextHopsId) and
// no new id should be allocated for the same nhop set. This is the
// no-op-replace contract that prevents unnecessary HW deltas + nhop-id
// waste on every reconfigure.
TEST_F(RibMySidNextHopTest, reconfigureIdenticalMySidIsNoOp) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  auto mySidConfig = makeNodeMySidConfig(0x100, "2001:db8::1");

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
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  const auto idBefore = getUnresolveId(prefix);
  ASSERT_TRUE(idBefore.has_value());

  // Re-apply the exact same config. The id must be preserved.
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
      convertMySidConfig(mySidConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto idAfter = getUnresolveId(prefix);
  ASSERT_TRUE(idAfter.has_value());
  EXPECT_EQ(*idAfter, *idBefore)
      << "Identical reconfigure churned the unresolveNextHopsId — "
         "applyStaticMySids should leave unchanged entries alone";
}

// Mixed reconfigure: changing only one of several entries should churn
// only that one. The unchanged entry's id stays put; the changed entry's
// id is reallocated.
TEST_F(RibMySidNextHopTest, reconfigureOnlyChangedMySidChurnsId) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  // Install two NODE entries.
  cfg::MySidConfig firstConfig;
  firstConfig.locatorPrefix() = kTestLocatorPrefix.str();
  cfg::NodeMySidConfig nodeA;
  nodeA.nodeAddress() = "2001:db8::1";
  cfg::MySidEntryConfig entryA;
  entryA.set_node(nodeA);
  firstConfig.entries()->emplace(0x100, entryA);

  cfg::NodeMySidConfig nodeB;
  nodeB.nodeAddress() = "2001:db8::2";
  cfg::MySidEntryConfig entryB;
  entryB.set_node(nodeB);
  firstConfig.entries()->emplace(0x200, entryB);

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
      convertMySidConfig(firstConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto p100 = makeSidPrefix("3001:db8:100::", 48);
  const auto p200 = makeSidPrefix("3001:db8:200::", 48);
  const auto idAOldOpt = getUnresolveId(p100);
  const auto idBOldOpt = getUnresolveId(p200);
  ASSERT_TRUE(idAOldOpt.has_value());
  ASSERT_TRUE(idBOldOpt.has_value());

  // Re-apply with entry A unchanged but entry B's nodeAddress flipped.
  cfg::MySidConfig secondConfig;
  secondConfig.locatorPrefix() = kTestLocatorPrefix.str();
  secondConfig.entries()->emplace(0x100, entryA);
  cfg::NodeMySidConfig nodeBNew;
  nodeBNew.nodeAddress() = "2001:db8::99";
  cfg::MySidEntryConfig entryBNew;
  entryBNew.set_node(nodeBNew);
  secondConfig.entries()->emplace(0x200, entryBNew);

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
      convertMySidConfig(secondConfig, {}),
      noopFibUpdate,
      &switchState_);

  const auto idANewOpt = getUnresolveId(p100);
  const auto idBNewOpt = getUnresolveId(p200);
  ASSERT_TRUE(idANewOpt.has_value());
  ASSERT_TRUE(idBNewOpt.has_value());
  // A is unchanged → same id.
  EXPECT_EQ(*idANewOpt, *idAOldOpt);
  // B changed → its old id is gone (released) and a new id allocated.
  EXPECT_NE(*idBNewOpt, *idBOldOpt);
  // Old id for B should have been deallocated from the manager.
  auto manager = rib_->getNextHopIDManagerCopy();
  ASSERT_NE(manager, nullptr);
  EXPECT_FALSE(manager->getNextHopsIf(NextHopSetID(*idBOldOpt)).has_value())
      << "Old next-hop set for changed entry B was not released";
}

// Identical DECAP reconfigure (no next hops) must hit the "both empty"
// branch of mySidEntryUnchanged and be a no-op — covers the 0-nhop fast
// path that the NODE-only no-op test does not exercise.
TEST_F(RibMySidNextHopTest, reconfigureIdenticalDecapMySidIsNoOp) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};
  auto mySidConfig = makeDecapMySidConfig(0x100);
  auto applyOnce = [&]() {
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
        convertMySidConfig(mySidConfig, {}),
        noopFibUpdate,
        &switchState_);
  };
  applyOnce();
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  const auto table1 = rib_->getMySidTableCopy();
  ASSERT_NE(table1.find(prefix), table1.end());
  const auto type1 = table1.at(prefix).type().value();

  applyOnce();
  const auto table2 = rib_->getMySidTableCopy();
  ASSERT_NE(table2.find(prefix), table2.end());
  EXPECT_EQ(table2.at(prefix).type().value(), type1);
  EXPECT_FALSE(table2.at(prefix).unresolveNextHopsId().has_value());
}

// Reconfigure that flips isV6 on the same prefix must churn the entry,
// not be skipped — covers the isV6 branch of mySidEntryUnchanged.
TEST_F(RibMySidNextHopTest, reconfigureChangedIsV6ChurnsEntry) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};
  auto mkConfigWithIsV6 = [&](bool isV6) {
    cfg::MySidConfig cfg;
    cfg.locatorPrefix() = kTestLocatorPrefix.str();
    cfg::AdjacencyMySidConfig adj;
    adj.portName() = "Port-Channel301";
    adj.isV6() = isV6;
    cfg::MySidEntryConfig entry;
    entry.set_adjacency(std::move(adj));
    cfg.entries()->emplace(0x100, std::move(entry));
    return cfg;
  };
  // portName -> InterfaceID map: convertMySidConfig requires it for adjacency.
  std::unordered_map<std::string, InterfaceID> portMap{
      {"Port-Channel301", InterfaceID(301)}};
  auto applyWithIsV6 = [&](bool isV6) {
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
        convertMySidConfig(mkConfigWithIsV6(isV6), portMap),
        noopFibUpdate,
        &switchState_);
  };
  applyWithIsV6(true);
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  ASSERT_NE(
      rib_->getMySidTableCopy().find(prefix), rib_->getMySidTableCopy().end());
  EXPECT_TRUE(rib_->getMySidTableCopy().at(prefix).isV6().value_or(false));

  applyWithIsV6(false);
  EXPECT_FALSE(rib_->getMySidTableCopy().at(prefix).isV6().value_or(true))
      << "Changing isV6 should churn the entry; smart diff should NOT skip";
}

// Reconfigure that flips adjacencyInterfaceId on the same prefix must
// churn the entry — covers the adjacencyInterfaceId branch.
TEST_F(RibMySidNextHopTest, reconfigureChangedAdjacencyInterfaceIdChurnsEntry) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};
  auto mkConfigWithPort = [&](const std::string& portName) {
    cfg::MySidConfig cfg;
    cfg.locatorPrefix() = kTestLocatorPrefix.str();
    cfg::AdjacencyMySidConfig adj;
    adj.portName() = portName;
    adj.isV6() = true;
    cfg::MySidEntryConfig entry;
    entry.set_adjacency(std::move(adj));
    cfg.entries()->emplace(0x100, std::move(entry));
    return cfg;
  };
  std::unordered_map<std::string, InterfaceID> portMap{
      {"Port-Channel301", InterfaceID(301)},
      {"Port-Channel302", InterfaceID(302)}};
  auto applyWithPort = [&](const std::string& portName) {
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
        convertMySidConfig(mkConfigWithPort(portName), portMap),
        noopFibUpdate,
        &switchState_);
  };
  applyWithPort("Port-Channel301");
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  EXPECT_EQ(
      rib_->getMySidTableCopy().at(prefix).adjacencyInterfaceId().value_or(0),
      301);

  applyWithPort("Port-Channel302");
  EXPECT_EQ(
      rib_->getMySidTableCopy().at(prefix).adjacencyInterfaceId().value_or(0),
      302)
      << "Changing adjacencyInterfaceId should churn the entry";
}

// Pass 2's "non-STATIC_ROUTE entry at same CIDR" overwrite path. Existing
// reconfigurePreservesTeAgentMySid uses a different prefix and so does
// not exercise this branch. Here we put a TE_AGENT entry (with nhops) at
// the exact prefix the next config reconfigure targets, and verify:
//   - the old TE_AGENT entry's nhop id is released (no leak)
//   - the new entry has STATIC_ROUTE clientId
TEST_F(RibMySidNextHopTest, reconfigureOverwritesTeAgentAtSamePrefix) {
  RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes interfaceRoutes;
  interfaceRoutes[kRid] = {};

  // 1. Install a TE_AGENT (default clientId) entry with nhops at the prefix
  //    that the next reconfigure will target.
  rib_->update(
      scopeResolver(),
      {makeMySidEntryWithNextHops("3001:db8:100::", 48, {"2001:db8::1"})},
      {},
      "seed te_agent at config-target prefix",
      mySidToSwitchStateUpdate,
      &switchState_);
  const auto prefix = makeSidPrefix("3001:db8:100::", 48);
  const auto teAgentIdOpt = getUnresolveId(prefix);
  ASSERT_TRUE(teAgentIdOpt.has_value())
      << "Test setup: TE_AGENT entry should have an unresolveNextHopsId";

  // 2. Reconfigure with a STATIC_ROUTE DECAP entry at the same prefix —
  //    Pass 2 in applyStaticMySids should release the TE_AGENT entry's id
  //    and install the DECAP entry.
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
      convertMySidConfig(makeDecapMySidConfig(0x100), {}),
      noopFibUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  ASSERT_NE(table.find(prefix), table.end());
  EXPECT_EQ(table.at(prefix).clientId().value(), ClientID::STATIC_ROUTE);
  EXPECT_EQ(table.at(prefix).type().value(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_FALSE(table.at(prefix).unresolveNextHopsId().has_value());

  // The TE_AGENT entry's old id must have been released (refcount 1 → 0).
  auto manager = rib_->getNextHopIDManagerCopy();
  ASSERT_NE(manager, nullptr);
  EXPECT_FALSE(manager->getNextHopsIf(NextHopSetID(*teAgentIdOpt)).has_value())
      << "Pass 2 silently leaked the overwritten TE_AGENT entry's nhop id";
}

// Tests for the update(MySidWithNextHops) / updateAsync(MySidWithNextHops)
// overloads — pre-built MySid state objects + parallel next-hop sets, used by
// the config + neighbor-observer paths.
class RibMySidWithNextHopsUpdateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    rib_ = std::make_unique<RoutingInformationBase>();
    switchState_ = std::make_shared<SwitchState>();
    switchState_->publish();
    rib_->ensureVrf(kRid);
  }

  void TearDown() override {
    // Reset the global gflag we toggled in SetUp so it doesn't leak into
    // tests that follow in the same binary.
    FLAGS_enable_nexthop_id_manager = false;
  }

  std::vector<MySidWithNextHops> makePair(
      const std::string& addr,
      uint8_t len,
      MySidType type = MySidType::DECAPSULATE_AND_LOOKUP,
      RouteNextHopSet nhops = {}) {
    auto mySid = makeMySid(makeSidPrefix(addr, len), type);
    mySid->setClientId(ClientID::STATIC_ROUTE);
    return {{std::move(mySid), std::move(nhops)}};
  }

  std::unique_ptr<RoutingInformationBase> rib_;
  std::shared_ptr<SwitchState> switchState_;
};

TEST_F(RibMySidWithNextHopsUpdateTest, syncUpdateAddsEntry) {
  rib_->update(
      scopeResolver(),
      makePair("3001:db8:1::", 48),
      {} /* toDelete */,
      "syncAdd",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  EXPECT_EQ(table.size(), 1);
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:1::", 48)), table.end());
}

TEST_F(RibMySidWithNextHopsUpdateTest, syncUpdateAllocatesNextHopSetId) {
  RouteNextHopSet nhops{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  rib_->update(
      scopeResolver(),
      makePair("3001:db8:1::", 48, MySidType::NODE_MICRO_SID, std::move(nhops)),
      {},
      "syncAddWithNhops",
      mySidToSwitchStateUpdate,
      &switchState_);

  const auto table = rib_->getMySidTableCopy();
  const auto p = makeSidPrefix("3001:db8:1::", 48);
  ASSERT_NE(table.find(p), table.end());
  EXPECT_TRUE(table.at(p).unresolveNextHopsId().has_value());
}

TEST_F(RibMySidWithNextHopsUpdateTest, syncUpdateDeletesEntry) {
  rib_->update(
      scopeResolver(),
      makePair("3001:db8:1::", 48),
      {},
      "add",
      mySidToSwitchStateUpdate,
      &switchState_);
  ASSERT_EQ(rib_->getMySidTableCopy().size(), 1);

  rib_->update(
      scopeResolver(),
      std::vector<MySidWithNextHops>{},
      {toIpPrefix("3001:db8:1::", 48)},
      "delete",
      mySidToSwitchStateUpdate,
      &switchState_);
  EXPECT_EQ(rib_->getMySidTableCopy().size(), 0);
}

TEST_F(RibMySidWithNextHopsUpdateTest, syncUpdatePropagatesException) {
  // Sync update must rethrow exceptions raised on the rib event-base thread —
  // captured via std::exception_ptr since folly's runInFbossEventBaseThread-
  // AndWait does not propagate.
  FailMySidUpdate failUpdate;
  EXPECT_THROW(
      rib_->update(
          scopeResolver(),
          makePair("3001:db8:1::", 48),
          {},
          "syncFail",
          failUpdate,
          &switchState_),
      FbossHwUpdateError);
}

TEST_F(RibMySidWithNextHopsUpdateTest, asyncUpdateAddsEntryAfterDrain) {
  rib_->updateAsync(
      scopeResolver(),
      makePair("3001:db8:1::", 48),
      {},
      "asyncAdd",
      mySidToSwitchStateUpdate,
      &switchState_);

  // updateAsync is fire-and-forget — drain the rib event base before
  // observing state.
  rib_->waitForRibUpdates();

  const auto table = rib_->getMySidTableCopy();
  EXPECT_EQ(table.size(), 1);
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:1::", 48)), table.end());
}

TEST_F(RibMySidWithNextHopsUpdateTest, asyncUpdateDeletesEntryAfterDrain) {
  // Seed an entry synchronously
  rib_->update(
      scopeResolver(),
      makePair("3001:db8:1::", 48),
      {},
      "syncAdd",
      mySidToSwitchStateUpdate,
      &switchState_);
  ASSERT_EQ(rib_->getMySidTableCopy().size(), 1);

  rib_->updateAsync(
      scopeResolver(),
      std::vector<MySidWithNextHops>{},
      {toIpPrefix("3001:db8:1::", 48)},
      "asyncDelete",
      mySidToSwitchStateUpdate,
      &switchState_);
  rib_->waitForRibUpdates();

  EXPECT_EQ(rib_->getMySidTableCopy().size(), 0);
}

TEST_F(RibMySidWithNextHopsUpdateTest, asyncUpdatesAreSerialized) {
  // Multiple async updates run sequentially on the same event-base thread and
  // are observable after a single drain.
  for (int i = 1; i <= 3; ++i) {
    rib_->updateAsync(
        scopeResolver(),
        makePair(folly::sformat("3001:db8:{}::", i), 48),
        {},
        folly::sformat("asyncAdd{}", i),
        mySidToSwitchStateUpdate,
        &switchState_);
  }
  rib_->waitForRibUpdates();

  const auto table = rib_->getMySidTableCopy();
  EXPECT_EQ(table.size(), 3);
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:1::", 48)), table.end());
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:2::", 48)), table.end());
  EXPECT_NE(table.find(makeSidPrefix("3001:db8:3::", 48)), table.end());
}

} // namespace facebook::fboss
