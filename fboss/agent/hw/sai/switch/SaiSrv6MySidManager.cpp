// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6MySidManager.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
#include "fboss/agent/FbossError.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/SwitchState.h"
#endif

namespace facebook::fboss {

SaiSrv6MySidManager::SaiSrv6MySidManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

namespace {
SaiMySidEntryTraits::CreateAttributes getMySidCreateAttributes(
    const MySid& mySid,
    const std::optional<SaiMySidEntryHandle::NextHopHandle>& nexthopHandle,
    SaiManagerTable* managerTable) {
  sai_int32_t endpointBehavior;
  switch (mySid.getType()) {
    case MySidType::ADJACENCY_MICRO_SID:
      endpointBehavior = SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA;
      break;
    case MySidType::NODE_MICRO_SID:
      endpointBehavior = SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UN;
      break;
    case MySidType::DECAPSULATE_AND_LOOKUP:
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 1)
      endpointBehavior = SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UDT46;
#else
      throw FbossError("Decapsulate with uSids requires SAI >= 1.16.0");
#endif
      break;
  }

  sai_object_id_t nextHopId = SAI_NULL_OBJECT_ID;
  if (nexthopHandle) {
    if (auto* nhgHandle = std::get_if<std::shared_ptr<SaiNextHopGroupHandle>>(
            &nexthopHandle.value())) {
      nextHopId = (*nhgHandle)->nextHopGroup->adapterKey();
    } else if (
        auto* managedNh = std::get_if<std::shared_ptr<ManagedMySidNextHop>>(
            &nexthopHandle.value())) {
      nextHopId = (*managedNh)->adapterKey();
    }
  }

  auto* vrHandle =
      managerTable->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  CHECK(vrHandle) << "No default virtual router";
  auto vrId = vrHandle->virtualRouter->adapterKey();

  sai_int32_t packetAction = SAI_PACKET_ACTION_FORWARD;
  if (mySid.getType() != MySidType::DECAPSULATE_AND_LOOKUP &&
      nextHopId == SAI_NULL_OBJECT_ID && nexthopHandle) {
    packetAction = SAI_PACKET_ACTION_DROP;
  }

  return SaiMySidEntryTraits::CreateAttributes{
      endpointBehavior,
      SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_FLAVOR_PSP_AND_USP,
      nextHopId,
      vrId,
      packetAction};
}
} // namespace

SaiMySidEntryTraits::AdapterHostKey getMySidAdapterHostKey(
    const MySid& mySid,
    SaiManagerTable* managerTable) {
  auto switchId = managerTable->switchManager().getSwitchSaiId();
  auto* vrHandle =
      managerTable->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  CHECK(vrHandle) << "No default virtual router";
  auto vrId = vrHandle->virtualRouter->adapterKey();
  auto [ip, maskLen] = mySid.getMySid();
  auto sid = ip.asV6();
  constexpr uint8_t kLocatorBlockLen = 32;
  uint8_t nonLocatorMaskLen = maskLen - kLocatorBlockLen;
  uint8_t nodeLen{0}, functionLen{0};

  switch (mySid.getType()) {
    case MySidType::ADJACENCY_MICRO_SID:
    case MySidType::DECAPSULATE_AND_LOOKUP:
      functionLen = nonLocatorMaskLen;
      break;
    case MySidType::NODE_MICRO_SID:
      nodeLen = nonLocatorMaskLen;
      break;
  }
  return SaiMySidEntryTraits::AdapterHostKey(
      switchId,
      vrId,
      kLocatorBlockLen,
      nodeLen,
      functionLen,
      0 /* argsLen */,
      sid);
}

ManagedMySidNextHop::ManagedMySidNextHop(
    SaiSrv6MySidManager* manager,
    SaiMySidEntryTraits::AdapterHostKey mySidKey,
    std::shared_ptr<ManagedNextHop<SaiIpNextHopTraits>> managedNextHop)
    : detail::SaiObjectEventSubscriber<SaiIpNextHopTraits>(
          managedNextHop->adapterHostKey()),
      manager_(manager),
      mySidKey_(std::move(mySidKey)),
      managedNextHop_(managedNextHop) {}

void ManagedMySidNextHop::afterCreate(
    ManagedMySidNextHop::PublisherObject nexthop) {
  this->setPublisherObject(nexthop);
  auto entry = manager_->getMySidObject(mySidKey_);
  if (!entry) {
    // managed next hop is created before my_sid entry.
    // while creating my_sid entry, managed next hop id must be used in create
    return;
  }
  entry->setAttribute(
      SaiMySidEntryTraits::Attributes::NextHopId{nexthop->adapterKey()});
  entry->setAttribute(
      SaiMySidEntryTraits::Attributes::PacketAction{SAI_PACKET_ACTION_FORWARD});
}

void ManagedMySidNextHop::beforeRemove() {
  auto entry = manager_->getMySidObject(mySidKey_);
  if (entry) {
    entry->setAttribute(
        SaiMySidEntryTraits::Attributes::NextHopId{SAI_NULL_OBJECT_ID});
    entry->setAttribute(
        SaiMySidEntryTraits::Attributes::PacketAction{SAI_PACKET_ACTION_DROP});
  }
  this->setPublisherObject(nullptr);
}

sai_object_id_t ManagedMySidNextHop::adapterKey() const {
  if (auto* nexthop = managedNextHop_->getSaiObject()) {
    return nexthop->adapterKey();
  }
  return SAI_NULL_OBJECT_ID;
}

std::shared_ptr<SaiObject<SaiMySidEntryTraits>>
SaiSrv6MySidManager::getMySidObject(
    const SaiMySidEntryTraits::AdapterHostKey& key) {
  return saiStore_->get<SaiMySidEntryTraits>().get(key);
}

void SaiSrv6MySidManager::addMySidEntry(
    const std::shared_ptr<MySid>& mySid,
    const std::shared_ptr<SwitchState>& state) {
  auto adapterHostKey = getMySidAdapterHostKey(*mySid, managerTable_);
  if (handles_.find(adapterHostKey) != handles_.end()) {
    throw FbossError("MySid entry already exists for ", mySid->getID());
  }

  if (!mySid->resolved()) {
    XLOG(DBG2) << "Skipping MySid entry " << mySid->getID()
               << " without resolved next hop";
    return;
  }

  std::optional<SaiMySidEntryHandle::NextHopHandle> nexthopHandle;

  auto resolvedNextHopsId = mySid->getResolvedNextHopsId();
  if (resolvedNextHopsId) {
    auto nhops = getNextHops(state, static_cast<int64_t>(*resolvedNextHopsId));
    if (nhops.size() > 1) {
      RouteNextHopSet nhopSet(nhops.begin(), nhops.end());
      auto nextHopGroupHandle =
          managerTable_->nextHopGroupManager().incRefOrAddNextHopGroup(
              SaiNextHopGroupKey(nhopSet, std::nullopt));
      nexthopHandle = nextHopGroupHandle;
    } else if (nhops.size() == 1) {
      auto resolvedNh = folly::poly_cast<ResolvedNextHop>(nhops.front());
      auto managedSaiNextHop =
          managerTable_->nextHopManager().addManagedSaiNextHop(resolvedNh);
      auto* ipNextHop =
          std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedSaiNextHop);
      CHECK(ipNextHop) << "Expected IP next hop for MySid entry "
                       << mySid->getID();
      auto managedMySidNextHop = std::make_shared<ManagedMySidNextHop>(
          this, adapterHostKey, *ipNextHop);
      SaiObjectEventPublisher::getInstance()
          ->get<SaiIpNextHopTraits>()
          .subscribe(managedMySidNextHop);
      nexthopHandle = managedMySidNextHop;
    } else {
      throw FbossError("Resolved nhops Id set, but no next hops found");
    }
  }

  auto createAttributes =
      getMySidCreateAttributes(*mySid, nexthopHandle, managerTable_);
  auto& store = saiStore_->get<SaiMySidEntryTraits>();
  auto mySidEntry = store.setObject(adapterHostKey, createAttributes);

  auto handle = std::make_unique<SaiMySidEntryHandle>();
  if (nexthopHandle) {
    handle->nexthopHandle = nexthopHandle.value();
  }
  handle->mySidEntry = mySidEntry;
  handles_.emplace(adapterHostKey, std::move(handle));
}

void SaiSrv6MySidManager::removeMySidEntry(
    const std::shared_ptr<MySid>& mySid,
    const std::shared_ptr<SwitchState>& /*state*/) {
  if (!mySid->resolved()) {
    return;
  }
  auto key = getMySidAdapterHostKey(*mySid, managerTable_);
  auto itr = handles_.find(key);
  if (itr == handles_.end()) {
    throw FbossError("MySid entry does not exist for ", mySid->getID());
  }
  handles_.erase(itr);
}

void SaiSrv6MySidManager::changeMySidEntry(
    const std::shared_ptr<MySid>& oldMySid,
    const std::shared_ptr<MySid>& newMySid,
    const std::shared_ptr<SwitchState>& state) {
  removeMySidEntry(oldMySid, state);
  addMySidEntry(newMySid, state);
}

#endif

} // namespace facebook::fboss
