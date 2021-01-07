// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/LabelForwardingAction.h"

namespace {
using namespace facebook::fboss;
SaiInSegEntryHandle::NextHopHandle getNextHopHandle(
    SaiManagerTable* managerTable,
    const std::shared_ptr<LabelForwardingEntry>& swLabelFibEntry) {
  const auto& nexthops = swLabelFibEntry->getLabelNextHop().getNextHopSet();
  if (nexthops.size() > 0 &&
      nexthops.begin()->labelForwardingAction()->type() ==
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
    // TODO(pshaikh): use "mpls router interface" for this
    throw FbossError("pop and look up is not supported");
  }

  auto nextHopGroupHandle =
      managerTable->nextHopGroupManager().incRefOrAddNextHopGroup(nexthops);
  return nextHopGroupHandle;
}
} // namespace
namespace facebook::fboss {

void SaiInSegEntryManager::processAddedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& addedEntry) {
  SaiInSegEntryHandle handle;

  handle.nexthopHandle = getNextHopHandle(managerTable_, addedEntry);
  SaiInSegTraits::CreateAttributes createAttributes{
      SAI_PACKET_ACTION_FORWARD, // not supporting any other action now except
                                 // forward
      1, // always pop 1 label even for swap and push another label
      handle.nextHopAdapterKey()};

  SaiInSegTraits::InSegEntry inSegEntry{
      managerTable_->switchManager().getSwitchSaiId(),
      static_cast<sai_label_id_t>(addedEntry->getID())};
  if (saiInSegEntryTable_.find(inSegEntry) != saiInSegEntryTable_.end()) {
    throw FbossError(
        "label fib entry already exists for ", addedEntry->getID());
  }

  auto& store = SaiStore::getInstance()->get<SaiInSegTraits>();
  handle.inSegEntry = store.setObject(inSegEntry, createAttributes);
  saiInSegEntryTable_.emplace(inSegEntry, handle);
}

void SaiInSegEntryManager::processChangedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& /*oldEntry*/,
    const std::shared_ptr<LabelForwardingEntry>& newEntry) {
  SaiInSegTraits::InSegEntry inSegEntry{
      managerTable_->switchManager().getSwitchSaiId(),
      static_cast<sai_label_id_t>(newEntry->getID())};
  auto itr = saiInSegEntryTable_.find(inSegEntry);
  if (itr == saiInSegEntryTable_.end()) {
    throw FbossError(
        "label fib entry already does not exist for ", newEntry->getID());
  }

  itr->second.nexthopHandle = getNextHopHandle(managerTable_, newEntry);

  SaiInSegTraits::CreateAttributes newAttributes{
      SAI_PACKET_ACTION_FORWARD, 1, itr->second.nextHopAdapterKey()};
  auto& store = SaiStore::getInstance()->get<SaiInSegTraits>();
  itr->second.inSegEntry = store.setObject(inSegEntry, newAttributes);
}

void SaiInSegEntryManager::processRemovedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& removedEntry) {
  auto inSegEntry = SaiInSegTraits::InSegEntry(
      managerTable_->switchManager().getSwitchSaiId(), removedEntry->getID());
  auto itr = saiInSegEntryTable_.find(inSegEntry);
  if (itr == saiInSegEntryTable_.end()) {
    throw FbossError(
        "label fib entry already does not exist for ", removedEntry->getID());
  }
  saiInSegEntryTable_.erase(inSegEntry);
}

const SaiInSegEntryHandle* SaiInSegEntryManager::getInSegEntryHandle(
    LabelForwardingEntry::Label label) const {
  auto inSegEntry = SaiInSegTraits::InSegEntry(
      managerTable_->switchManager().getSwitchSaiId(), label);
  const auto& itr = saiInSegEntryTable_.find(inSegEntry);
  if (itr == saiInSegEntryTable_.end()) {
    return nullptr;
  }
  return &(itr->second);
}

template <typename NextHopTraitsT>
ManagedInSegNextHop<NextHopTraitsT>::ManagedInSegNextHop(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform,
    SaiInSegTraits::AdapterHostKey inSegKey,
    std::shared_ptr<ManagedNextHop<NextHopTraitsT>> managedNextHop)
    : detail::SaiObjectEventSubscriber<NextHopTraitsT>(
          managedNextHop->adapterHostKey()),
      managerTable_(managerTable),
      platform_(platform),
      inSegKey_(std::move(inSegKey)),
      managedNextHop_(managedNextHop) {}

template <typename NextHopTraitsT>
void ManagedInSegNextHop<NextHopTraitsT>::afterCreate(
    ManagedInSegNextHop<NextHopTraitsT>::PublisherObject nexthop) {
  this->setPublisherObject(nexthop);
  // set entry to next hop
  auto entry = SaiStore::getInstance()->get<SaiInSegTraits>().get(inSegKey_);
  if (!entry) {
    // entry is not yet created.
    return;
  }
  auto attributes = entry->attributes();
  sai_object_id_t nextHopId = nexthop->adapterKey();
  std::get<std::optional<SaiInSegTraits::Attributes::NextHopId>>(attributes) =
      nextHopId;
  entry->setAttributes(attributes);
}

template <typename NextHopTraitsT>
void ManagedInSegNextHop<NextHopTraitsT>::beforeRemove() {
  // set entry to NULL
  auto entry = SaiStore::getInstance()->get<SaiInSegTraits>().get(inSegKey_);
  auto attributes = entry->attributes();

  std::get<std::optional<SaiInSegTraits::Attributes::NextHopId>>(attributes) =
      SAI_NULL_OBJECT_ID;
  entry->setAttributes(attributes);
  this->setPublisherObject(nullptr);
}

template <typename NextHopTraitsT>
sai_object_id_t ManagedInSegNextHop<NextHopTraitsT>::adapterKey() const {
  if (auto* nexthop = managedNextHop_->getSaiObject()) {
    return nexthop->adapterKey();
  }
  return SAI_NULL_OBJECT_ID;
}

sai_object_id_t SaiInSegEntryHandle::nextHopAdapterKey() const {
  return std::visit(
      [](auto& handle) { return handle->adapterKey(); }, nexthopHandle);
}

std::shared_ptr<SaiNextHopGroupHandle> SaiInSegEntryHandle::nextHopGroupHandle()
    const {
  auto* nextHopGroupHandle =
      std::get_if<std::shared_ptr<SaiNextHopGroupHandle>>(&nexthopHandle);
  if (!nextHopGroupHandle) {
    return nullptr;
  }
  return *nextHopGroupHandle;
}
} // namespace facebook::fboss
