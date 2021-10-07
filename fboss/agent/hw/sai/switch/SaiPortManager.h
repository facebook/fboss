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
#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSamplePacketManager.h"
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
class SaiStore;

using SaiPort = SaiObjectWithCounters<SaiPortTraits>;
using SaiPortSerdes = SaiObject<SaiPortSerdesTraits>;
using SaiPortConnector = SaiObject<SaiPortConnectorTraits>;

/*
 * Cache port mirror data from sw switch
 */
struct SaiPortMirrorInfo {
  std::optional<std::string> ingressMirror;
  std::optional<std::string> egressMirror;
  bool samplingMirror;
  SaiPortMirrorInfo() {}
  SaiPortMirrorInfo(
      std::optional<std::string> ingressMirror,
      std::optional<std::string> egressMirror,
      bool samplingMirror)
      : ingressMirror(ingressMirror),
        egressMirror(egressMirror),
        samplingMirror(samplingMirror) {}
  std::optional<std::string> getIngressMirror() {
    return ingressMirror;
  }
  std::optional<std::string> getEgressMirror() {
    return egressMirror;
  }
  bool isMirrorSampled() {
    return samplingMirror;
  }
};

/*
 * For Xphy we create system side port, line side port and a port connector
 * associating these two. The Line side port is used for all subsequent MacSec
 * programming and it is kept in PortHandle's port, the system side Sai port is
 * kept in saiPort and the port connector in connector field.
 */
struct SaiPortHandle {
  ~SaiPortHandle();
  std::shared_ptr<SaiPort> port;
  std::shared_ptr<SaiPort> sysPort;
  std::shared_ptr<SaiPortSerdes> serdes;
  std::shared_ptr<SaiPortConnector> connector;
  std::shared_ptr<SaiBridgePort> bridgePort;
  std::vector<SaiQueueHandle*> configuredQueues;
  std::shared_ptr<SaiSamplePacket> ingressSamplePacket;
  std::shared_ptr<SaiSamplePacket> egressSamplePacket;
  SaiQueueHandles queues;
  SaiPortMirrorInfo mirrorInfo;
};

class SaiPortManager {
  using Handles = folly::F14FastMap<PortID, std::unique_ptr<SaiPortHandle>>;
  using Stats = folly::F14FastMap<PortID, std::unique_ptr<HwPortFb303Stats>>;

 public:
  SaiPortManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform,
      ConcurrentIndices* concurrentIndices_);
  ~SaiPortManager();
  PortSaiId addPort(const std::shared_ptr<Port>& swPort);
  void removePort(const std::shared_ptr<Port>& swPort);
  void changePort(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  bool createOnlyAttributeChanged(
      const SaiPortTraits::CreateAttributes& oldAttributes,
      const SaiPortTraits::CreateAttributes& newAttributes);

  SaiPortTraits::CreateAttributes attributesFromSwPort(
      const std::shared_ptr<Port>& swPort,
      bool lineSide = false) const;

  SaiPortSerdesTraits::CreateAttributes serdesAttributesFromSwPinConfigs(
      PortSaiId portSaid,
      const std::vector<phy::PinConfig>& pinConfigs);

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

  void updateStats(PortID portID, bool updateWatermarks = false);

  void clearStats(PortID portID);

  void programMirrorOnAllPorts(
      const std::string& mirrorName,
      MirrorAction action);

  void addBridgePort(const std::shared_ptr<Port>& port);
  void changeBridgePort(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  bool isUp(PortID portID) const;

  bool isUp(PortSaiId saiPortId) const;

  void setPtpTcEnable(bool enable);
  bool isPtpTcEnabled() const;

 private:
  PortSaiId addPortImpl(const std::shared_ptr<Port>& swPort);
  void addRemovedHandle(PortID portID);
  void removeRemovedHandleIf(PortID portID);
  void releasePorts();

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
  void programSerdes(
      std::shared_ptr<SaiPort> saiPort,
      std::shared_ptr<Port> swPort,
      SaiPortHandle* portHandle);
  void programSampling(
      PortID portId,
      SamplePacketDirection direction,
      SamplePacketAction action,
      uint64_t sampleRate,
      std::optional<cfg::SampleDestination> sampleDestination);
  void programMirror(
      PortID portId,
      MirrorDirection direction,
      MirrorAction action,
      std::optional<std::string> mirrorId);
  void programSamplingMirror(
      PortID portId,
      MirrorDirection direction,
      MirrorAction action,
      std::optional<std::string> mirrorId);
  void addMirror(const std::shared_ptr<Port>& swPort);
  void removeMirror(const std::shared_ptr<Port>& swPort);
  void addSamplePacket(const std::shared_ptr<Port>& swPort);
  void removeSamplePacket(const std::shared_ptr<Port>& swPort);
  void changeMirror(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void changeSamplePacket(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  bool checkPortSerdesAttributes(
      const SaiPortSerdesTraits::CreateAttributes& fromStore,
      const SaiPortSerdesTraits::CreateAttributes& fromSwPort);
  void programMacsec(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  // Pure virtual function cannot be called in destructor. Thus, cache it.
  bool removePortsAtExit_;
  ConcurrentIndices* concurrentIndices_;
  Handles handles_;
  // on some platforms port can not be removed freely. on such platforms
  // retain removed port handle so it does not invoke remove port api.
  Handles removedHandles_;
  Stats portStats_;
  std::shared_ptr<SaiQosMap> globalDscpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToQueueQosMap_;

  std::optional<SaiPortTraits::Attributes::PtpMode> getPtpMode() const;
};

} // namespace facebook::fboss
