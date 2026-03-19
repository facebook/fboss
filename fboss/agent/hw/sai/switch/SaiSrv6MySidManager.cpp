// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6MySidManager.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/state/MySid.h"
#endif

namespace facebook::fboss {

SaiSrv6MySidManager::SaiSrv6MySidManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

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
  uint8_t locatorNodeLen = maskLen;
  return SaiMySidEntryTraits::AdapterHostKey(
      switchId,
      vrId,
      kLocatorBlockLen,
      locatorNodeLen,
      0 /* functionLen */,
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
  entry->setOptionalAttribute(
      SaiMySidEntryTraits::Attributes::NextHopId{nexthop->adapterKey()});
}

void ManagedMySidNextHop::beforeRemove() {
  auto entry = manager_->getMySidObject(mySidKey_);
  if (entry) {
    entry->setOptionalAttribute(
        SaiMySidEntryTraits::Attributes::NextHopId{SAI_NULL_OBJECT_ID});
    entry->setOptionalAttribute(
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
    const std::shared_ptr<MySid>& /*mySid*/) {
  // TODO: implement
}

void SaiSrv6MySidManager::removeMySidEntry(
    const std::shared_ptr<MySid>& mySid) {
  auto key = getMySidAdapterHostKey(*mySid, managerTable_);
  auto itr = handles_.find(key);
  if (itr == handles_.end()) {
    throw FbossError("MySid entry does not exist for ", mySid->getID());
  }
  handles_.erase(itr);
}

void SaiSrv6MySidManager::changeMySidEntry(
    const std::shared_ptr<MySid>& oldMySid,
    const std::shared_ptr<MySid>& newMySid) {
  removeMySidEntry(oldMySid);
  addMySidEntry(newMySid);
}

#endif

} // namespace facebook::fboss
