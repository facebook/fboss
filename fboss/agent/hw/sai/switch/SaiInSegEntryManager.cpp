// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/LabelForwardingAction.h"

namespace {
using namespace facebook::fboss;
std::shared_ptr<SaiNextHopGroupHandle> getNextHopGroupHandle(
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

  auto nextHopGroupHandle = getNextHopGroupHandle(managerTable_, addedEntry);

  SaiInSegTraits::CreateAttributes createAttributes{
      SAI_PACKET_ACTION_FORWARD, // not supporting any other action now except
                                 // forward
      1, // always pop 1 label even for swap and push another label
      nextHopGroupHandle->nextHopGroup->adapterKey()};

  SaiInSegTraits::InSegEntry inSegEntry{
      managerTable_->switchManager().getSwitchSaiId(),
      static_cast<sai_label_id_t>(addedEntry->getID())};
  if (saiInSegEntryTable_.find(inSegEntry) != saiInSegEntryTable_.end()) {
    throw FbossError(
        "label fib entry already exists for ", addedEntry->getID());
  }

  auto& store = SaiStore::getInstance()->get<SaiInSegTraits>();
  handle.inSegEntry = store.setObject(inSegEntry, createAttributes);
  handle.nextHopGroupHandle = nextHopGroupHandle;

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

  auto nextHopGroupHandle = getNextHopGroupHandle(managerTable_, newEntry);

  SaiInSegTraits::CreateAttributes newAttributes{
      SAI_PACKET_ACTION_FORWARD,
      1,
      nextHopGroupHandle->nextHopGroup->adapterKey()};
  auto& store = SaiStore::getInstance()->get<SaiInSegTraits>();
  itr->second.inSegEntry = store.setObject(inSegEntry, newAttributes);
  itr->second.nextHopGroupHandle = nextHopGroupHandle;
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

} // namespace facebook::fboss
