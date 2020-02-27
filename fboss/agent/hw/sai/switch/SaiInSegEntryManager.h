// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NodeMapDelta.h"

namespace facebook::fboss {

class LabelForwardingEntry;
class SaiManagerTable;
class SaiPlatform;

class SaiInSegEntryManager {
 public:
  SaiInSegEntryManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform)
      : managerTable_(managerTable), platform_(platform) {}

  void processInSegEntryDelta(
      const NodeMapDelta<LabelForwardingInformationBase>& delta);

 private:
  void processAddedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& addedEntry);
  void processChangedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& oldEntry,
      const std::shared_ptr<LabelForwardingEntry>& newEntry);
  void processRemovedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& removedEntry);

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};
} // namespace facebook::fboss
