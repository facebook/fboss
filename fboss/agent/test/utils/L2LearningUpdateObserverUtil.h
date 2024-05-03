// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/L2LearnEventObserver.h"
#include "fboss/agent/test/AgentEnsemble.h"

#include <folly/Optional.h>
#include <condition_variable>
#include <optional>
#include <vector>

namespace facebook::fboss {

class SwSwitch;
class L2Entry;
enum class L2EntryUpdateType;

class L2LearningUpdateObserverUtil : public L2LearnEventObserverIf {
 public:
  void startObserving(AgentEnsemble* ensemble);
  void stopObserving();

  void reset();

  void l2LearningUpdateReceived(
      const L2Entry& l2Entry,
      const L2EntryUpdateType& l2EntryUpdateType) noexcept override;
  std::vector<std::pair<L2Entry, L2EntryUpdateType>> waitForLearningUpdates(
      int numUpdates = 1,
      std::optional<int> secondsToWait = std::nullopt);

 private:
  AgentEnsemble* ensemble_{nullptr};
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::pair<L2Entry, L2EntryUpdateType>> data_;
};
} // namespace facebook::fboss
