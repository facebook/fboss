/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"
#include "folly/container/F14Set.h"

namespace facebook::fboss {

class ConcurrentIndices;
class SaiManagerTable;
class SaiPlatform;
class HwPortFb303Stats;
class QosPolicy;

using SaiPort = SaiObjectWithCounters<SaiPortTraits>;
using SaiPortSerdes = SaiObject<SaiPortSerdesTraits>;

struct SaiPortHandle {
  std::shared_ptr<SaiPort> port;
  std::shared_ptr<SaiPortSerdes> serdes;
  std::shared_ptr<SaiBridgePort> bridgePort;
  std::vector<SaiQueueHandle*> configuredQueues;
  SaiQueueHandles queues;
};

class SaiPortManager {
  using Handles = folly::F14FastMap<PortID, std::unique_ptr<SaiPortHandle>>;
  using Stats = folly::F14FastMap<PortID, std::unique_ptr<HwPortFb303Stats>>;

 public:
  SaiPortManager(
      SaiManagerTable* managerTable,
      SaiPlatform* platform,
      ConcurrentIndices* concurrentIndices_);
  ~SaiPortManager();
  PortSaiId addPort(const std::shared_ptr<Port>& swPort);
  void removePort(const std::shared_ptr<Port>& swPort);
  void changePort(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  SaiPortTraits::CreateAttributes attributesFromSwPort(
      const std::shared_ptr<Port>& swPort) const;

  SaiPortSerdesTraits::CreateAttributes serdesAttributesFromSwPort(
      PortSaiId portSaid,
      const std::shared_ptr<Port>& swPort);

  const SaiPortHandle* getPortHandle(PortID swId) const;
  SaiPortHandle* getPortHandle(PortID swId);
  const SaiQueueHandle* getQueueHandle(
      PortID swId,
      const SaiQueueConfig& saiQueueConfig) const;
  SaiQueueHandle* getQueueHandle(
      PortID swId,
      const SaiQueueConfig& saiQueueConfig);
  std::map<PortID, HwPortStats> getPortStats() const;
  void changeQueue(
      PortID swId,
      const QueueConfig& oldQueueConfig,
      const QueueConfig& newQueueConfig);

  const HwPortFb303Stats* getLastPortStat(PortID port) const;

  const Stats& getLastPortStats() const {
    return portStats_;
  }

  cfg::PortSpeed getMaxSpeed(PortID port) const;
  Handles::const_iterator begin() const {
    return handles_.begin();
  }
  Handles::const_iterator end() const {
    return handles_.end();
  }
  void setQosPolicy();
  void clearQosPolicy();

  std::shared_ptr<PortMap> reconstructPortsFromStore() const;

  std::shared_ptr<Port> swPortFromAttributes(
      SaiPortTraits::CreateAttributes attributees) const;

  void updateStats(PortID portID);

  void clearStats(PortID portID);

  std::optional<cfg::L2LearningMode> getL2LearningMode() const {
    return l2LearningMode_;
  }
  void setL2LearningMode(cfg::L2LearningMode l2LearningMode);

 private:
  void addRemovedHandle(PortID portID);
  void removeRemovedHandleIf(PortID portID);

  void setQosMaps(
      QosMapSaiId dscpToTc,
      QosMapSaiId tcToQueue,
      const folly::F14FastSet<PortID>& ports);

  void setQosMapsOnAllPorts(QosMapSaiId dscpToTc, QosMapSaiId tcToQueue);
  const std::vector<sai_stat_id_t>& supportedStats() const;
  SaiPortHandle* getPortHandleImpl(PortID swId) const;
  SaiQueueHandle* getQueueHandleImpl(
      PortID swId,
      const SaiQueueConfig& saiQueueConfig) const;
  void loadPortQueues(SaiPortHandle* portHandle);
  std::shared_ptr<SaiPortSerdes> programSerdes(
      std::shared_ptr<SaiPort> saiPort,
      std::shared_ptr<Port> swPort);
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  ConcurrentIndices* concurrentIndices_;
  Handles handles_;
  // on some platforms port can not be removed freely. on such platforms retain
  // removed port handle so it does not invoke remove port api.
  Handles removedHandles_;
  Stats portStats_;
  std::shared_ptr<SaiQosMap> globalDscpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToQueueQosMap_;
  std::optional<cfg::L2LearningMode> l2LearningMode_{std::nullopt};
};

} // namespace facebook::fboss
