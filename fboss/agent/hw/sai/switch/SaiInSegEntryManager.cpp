// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/DeltaFunctions.h"

namespace facebook::fboss {

void SaiInSegEntryManager::processInSegEntryDelta(
    const NodeMapDelta<LabelForwardingInformationBase>& delta) {
  /* TODO(pshaikh): remove below once both are used */
  CHECK(managerTable_);
  CHECK(platform_);
  DeltaFunctions::forEachAdded(delta, [this](const auto& labelFibEntry) {
    processAddedInSegEntry(labelFibEntry);
  });

  DeltaFunctions::forEachChanged(
      delta,
      [this](const auto& oldLabelFibEntry, const auto& newLabelFibEntry) {
        processChangedInSegEntry(oldLabelFibEntry, newLabelFibEntry);
      });

  DeltaFunctions::forEachRemoved(delta, [this](const auto& labelFibEntry) {
    processRemovedInSegEntry(labelFibEntry);
  });
}

void SaiInSegEntryManager::processAddedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& /*addedEntry*/) {
  // TODO(pshaikh): implement this
  throw FbossError("processAddedInSegEntry not implemented");
}

void SaiInSegEntryManager::processChangedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& /*oldEntry*/,
    const std::shared_ptr<LabelForwardingEntry>& /*ewEntry*/) {
  // TODO(pshaikh): implement this
  throw FbossError("processChangedInSegEntry not implemented");
}

void SaiInSegEntryManager::processRemovedInSegEntry(
    const std::shared_ptr<LabelForwardingEntry>& /*removedEntry*/) {
  // TODO(pshaikh): implement this
  throw FbossError("processChangedInSegEntry not implemented");
}

} // namespace facebook::fboss
