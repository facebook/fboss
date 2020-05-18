// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NodeMapDelta.h"

#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include "folly/container/F14Map.h"

#include <memory>
#include <mutex>

namespace facebook::fboss {

class LabelForwardingEntry;
class SaiManagerTable;
class SaiPlatform;
struct SaiNextHopGroupHandle;

using SaiInSegEntry = SaiObject<SaiInSegTraits>;

struct SaiInSegEntryHandle {
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle;
  std::shared_ptr<SaiInSegEntry> inSegEntry;
};

class SaiInSegEntryManager {
 public:
  SaiInSegEntryManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform)
      : managerTable_(managerTable), platform_(platform) {}

  // for tests only
  const SaiInSegEntryHandle* getInSegEntryHandle(
      LabelForwardingEntry::Label label) const;

  void processAddedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& addedEntry);
  void processChangedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& oldEntry,
      const std::shared_ptr<LabelForwardingEntry>& newEntry);
  void processRemovedInSegEntry(
      const std::shared_ptr<LabelForwardingEntry>& removedEntry);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::
      F14FastMap<typename SaiInSegTraits::AdapterHostKey, SaiInSegEntryHandle>
          saiInSegEntryTable_;
};
} // namespace facebook::fboss
