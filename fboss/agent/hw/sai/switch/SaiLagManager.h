// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook::fboss {

struct ConcurrentIndices;

using SaiLag = SaiObject<SaiLagTraits>;
using SaiLagMember = SaiObject<SaiLagMemberTraits>;

struct SaiLagHandle {
  std::shared_ptr<SaiLag> lag{};
  std::map<PortSaiId, std::shared_ptr<SaiLagMember>> members{};
  std::shared_ptr<SaiBridgePort> bridgePort{};
  uint32_t minimumLinkCount{0};
};

class SaiManagerTable;
class SaiPlatform;

class SaiLagManager {
 public:
  using Handles =
      folly::F14FastMap<AggregatePortID, std::unique_ptr<SaiLagHandle>>;
  SaiLagManager(
      SaiManagerTable* managerTable,
      SaiPlatform* platform,
      ConcurrentIndices* concurrentIndices)
      : managerTable_(managerTable),
        platform_(platform),
        concurrentIndices_(concurrentIndices) {}
  ~SaiLagManager();
  void addLag(const std::shared_ptr<AggregatePort>& aggregatePort);
  void removeLag(const std::shared_ptr<AggregatePort>& aggregatePort);
  void changeLag(
      const std::shared_ptr<AggregatePort>& oldAggregatePort,
      const std::shared_ptr<AggregatePort>& newAggregatePort);

  SaiLagHandle* getLagHandleIf(AggregatePortID aggregatePortID) const;
  SaiLagHandle* getLagHandle(AggregatePortID aggregatePortID) const;
  bool isMinimumLinkMet(AggregatePortID aggregatePortID) const;
  void removeMember(AggregatePortID aggPort, PortID subPort);

  uint8_t getLagCount() const {
    return handles_.size();
  }
  uint8_t getLagMemberCount(AggregatePortID aggPort) const;

 private:
  std::pair<PortSaiId, std::shared_ptr<SaiLagMember>> addMember(
      const std::shared_ptr<SaiLag>& lag,
      AggregatePortID aggPort,
      PortID subPort);
  void removeLagHandle(AggregatePortID aggPort, SaiLagHandle* handle);

  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  ConcurrentIndices* concurrentIndices_;
  Handles handles_;
};

} // namespace facebook::fboss
