// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook::fboss {

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
  SaiLagManager(SaiManagerTable* managerTable, SaiPlatform* platform)
      : managerTable_(managerTable), platform_(platform) {}
  void addLag(const std::shared_ptr<AggregatePort>& aggregatePort);
  void removeLag(const std::shared_ptr<AggregatePort>& aggregatePort);
  void changeLag(
      const std::shared_ptr<AggregatePort>& oldAggregatePort,
      const std::shared_ptr<AggregatePort>& newAggregatePort);

 private:
  std::pair<PortSaiId, std::shared_ptr<SaiLagMember>> addMember(
      const std::shared_ptr<SaiLag>& lag,
      PortID subPort);
  void removeMember(AggregatePortID aggPort, PortID subPort);

  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  Handles handles_;
};

} // namespace facebook::fboss
