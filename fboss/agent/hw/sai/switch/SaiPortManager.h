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
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
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

struct ConcurrentIndices;
struct SaiIngressPriorityGroupHandleAndProfile;
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
  std::shared_ptr<SaiPortSerdes> sysSerdes;
  std::shared_ptr<SaiPortConnector> connector;
  std::shared_ptr<SaiBridgePort> bridgePort;
  std::vector<SaiQueueHandle*> configuredQueues;
  std::shared_ptr<SaiSamplePacket> ingressSamplePacket;
  std::shared_ptr<SaiSamplePacket> egressSamplePacket;
  SaiQueueHandles queues;

  void resetQueues();
  SaiPortMirrorInfo mirrorInfo;
  folly::F14FastMap<
      IngressPriorityGroupID,
      SaiIngressPriorityGroupHandleAndProfile>
      configuredIngressPriorityGroups;
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
  void resetQueues();
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

  void attributesFromSaiStore(SaiPortTraits::CreateAttributes& attributes);

  SaiPortSerdesTraits::CreateAttributes serdesAttributesFromSwPinConfigs(
      PortSaiId portSaid,
      const std::vector<phy::PinConfig>& pinConfigs,
      const std::shared_ptr<SaiPortSerdes>& serdes);

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
      const std::shared_ptr<Port>& swPort,
      const QueueConfig& oldQueueConfig,
      const QueueConfig& newQueueConfig);

  const HwPortFb303Stats* getLastPortStat(PortID port) const;

  std::map<PortID, FabricEndpoint> getFabricReachability() const;
  const Stats& getLastPortStats() const {
    return portStats_;
  }

  cfg::PortSpeed getMaxSpeed(PortID port) const;
  // getSpeed returns the currently configured speed
  cfg::PortSpeed getSpeed(PortID port) const;
  Handles::const_iterator begin() const {
    return handles_.begin();
  }
  Handles::const_iterator end() const {
    return handles_.end();
  }
  void setQosPolicy();
  void clearQosPolicy();

  std::shared_ptr<PortMap> reconstructPortsFromStore(
      cfg::SwitchType switchType) const;

  std::shared_ptr<Port> swPortFromAttributes(
      SaiPortTraits::CreateAttributes attributees,
      PortSaiId portSaiId,
      cfg::SwitchType switchType) const;

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

  void setPtpTcEnable(bool enable);
  bool isPtpTcEnabled() const;

  std::vector<sai_port_lane_eye_values_t> getPortEyeValues(
      PortSaiId saiPortId) const;
  std::vector<sai_port_err_status_t> getPortErrStatus(
      PortSaiId saiPortId) const;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
  std::vector<sai_port_lane_latch_status_t> getRxSignalDetect(
      PortSaiId saiPortId,
      uint8_t numPmdLanes) const;
  std::vector<sai_port_lane_latch_status_t> getRxLockStatus(
      PortSaiId saiPortId,
      uint8_t numPmdLanes) const;
  std::vector<sai_port_lane_latch_status_t> getFecAlignmentLockStatus(
      PortSaiId saiPortId,
      uint8_t numFecLanes) const;
  std::optional<sai_latch_status_t> getPcsRxLinkStatus(
      PortSaiId saiPortId) const;
#endif

  void enableAfeAdaptiveMode(PortID portId);

  phy::FecMode getFECMode(PortID portId) const;

  phy::InterfaceType getInterfaceType(PortID portID) const;

  TransmitterTechnology getMedium(PortID portID) const;

  uint8_t getNumPmdLanes(PortSaiId saiPortId) const;
  void loadPortQueuesForAddedPort(const std::shared_ptr<Port>& swPort);
  void loadPortQueuesForChangedPort(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  cfg::PortType getPortType(PortID portId) const;

 private:
  PortSaiId addPortImpl(const std::shared_ptr<Port>& swPort);
  void changePortImpl(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void addRemovedHandle(PortID portID);
  void removeRemovedHandleIf(PortID portID);
  void releasePorts();
  void releasePortPfcBuffers();

  void setQosMaps(
      std::vector<std::pair<sai_qos_map_type_t, QosMapSaiId>>& qosMaps,
      const folly::F14FastSet<PortID>& ports);
  std::vector<std::pair<sai_qos_map_type_t, QosMapSaiId>>
  getNullSaiIdsForQosMaps();
  std::vector<std::pair<sai_qos_map_type_t, QosMapSaiId>> getSaiIdsForQosMaps();

  void setQosMapsOnAllPorts(
      std::vector<std::pair<sai_qos_map_type_t, QosMapSaiId>>& qosMaps);
  const std::vector<sai_stat_id_t>& supportedStats(PortID port);
  void fillInSupportedStats(PortID port);
  bool fecStatsSupported(PortID portID) const;
  SaiPortHandle* getPortHandleImpl(PortID swId) const;
  SaiQueueHandle* getQueueHandleImpl(
      PortID swId,
      const SaiQueueConfig& saiQueueConfig) const;
  void loadPortQueues(const Port& swPort);
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
  void resetSamplePacket(SaiPortHandle* portHandle);
  void programPfc(
      const std::shared_ptr<Port>& swPort,
      sai_uint8_t txPfc,
      sai_uint8_t rxPfc);
  std::pair<sai_uint8_t, sai_uint8_t> preparePfcConfigs(
      const std::shared_ptr<Port>& swPort);
  void addPfc(const std::shared_ptr<Port>& swPort);
  void changePfc(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void removePfc(const std::shared_ptr<Port>& swPort);
  void setPortType(PortID portId, cfg::PortType portType);
  void programPfcBuffers(const std::shared_ptr<Port>& swPort);
  void removePfcBuffers(const std::shared_ptr<Port>& swPort);
  void removeIngressPriorityGroupMappings(SaiPortHandle* portHandle);
  void applyPriorityGroupBufferProfile(
      const std::shared_ptr<Port>& swPort,
      std::shared_ptr<SaiBufferProfile> bufferProfile,
      IngressPriorityGroupSaiId ingressPgSaiId);
  std::vector<IngressPriorityGroupSaiId> getIngressPriorityGroupSaiIds(
      const std::shared_ptr<Port>& swPort);
  void changePortByRecreate(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  std::optional<FabricEndpoint> getFabricReachabilityForPort(
      const PortID& portId,
      const SaiPortHandle* portHandle) const;

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
  std::unordered_map<PortID, cfg::PortType> port2PortType_;
  std::unordered_map<PortID, std::vector<sai_stat_id_t>> port2SupportedStats_;
  std::unordered_map<PortID, std::shared_ptr<Port>> pendingNewPorts_;
  bool hwLaneListIsPmdLaneList_;
  bool tcToQueueMapAllowedOnPort_;
};

} // namespace facebook::fboss
