// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/MySidMapUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/MySidMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

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

IpPrefix toIpPrefix(const std::string& addr, uint8_t len) {
  IpPrefix prefix;
  prefix.ip() = facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
  prefix.prefixLength() = len;
  return prefix;
}

// Callback that uses MySidMapUpdater to apply RIB MySid to SwitchState.
StateDelta mySidToSwitchStateUpdate(
    const SwitchIdScopeResolver* resolver,
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
  mySid->setUnresolvedNextHop(std::nullopt);
  mySid->setResolvedNextHop(std::nullopt);
  return mySid;
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
      {},
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
      {},
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
      {},
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
          {},
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
      {},
      {},
      "empty mysid update",
      mySidToSwitchStateUpdate,
      &switchState);

  EXPECT_EQ(rib.getMySidTableCopy().size(), 0);
  // switchState should not change on empty update
  EXPECT_EQ(switchState->getMySids()->numNodes(), 0);
}
