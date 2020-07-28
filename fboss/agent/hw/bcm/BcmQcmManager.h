// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/state/QcmConfig.h"

extern "C" {
#include <bcm/flowtracker.h>
#include <bcm/l2.h>
}

// Currently QCM feature is only supported on sdk version 6.5.16
// TODO: rohitpuri
// Once the feature is more widely supported, these changes need to
// be revisted
#define BCM_VER_SUPPORT_QCM \
  (BCM_VER_MAJOR == 6) && (BCM_VER_MINOR == 5) && (BCM_VER_RELEASE == 16)

typedef std::set<int> QosQueueIds;

namespace facebook::fboss {

class BcmQcmCollector;
class QcmCfg;

class BcmQcmManager {
 public:
  explicit BcmQcmManager(BcmSwitch* hw);
  ~BcmQcmManager();
  void init(const std::shared_ptr<SwitchState>& swState);
  void stop();
  bool updateQcmMonitoredPortsIfNeeded(
      const Port2QosQueueIdMap& candidatePortMap);
  void flowLimitSet(int flowLimit);
  void processPortsForQcm(const std::shared_ptr<SwitchState>& swState);
  void processQcmConfigChanged(
      const std::shared_ptr<SwitchState> newSwState,
      const std::shared_ptr<SwitchState> oldSwState);

  // utility routines
  static bool isQcmSupported(BcmSwitch* hw);

  // utility routines used by tests
  uint64_t getIfpHitStatCounter(const int portId);
  uint64_t getIfpRedPktStatCounter(const int portId);
  uint32_t getLearnedFlowCount();
  static int getQcmFlowGroupId();
  bool isFlowTrackerDisabled();
  void setAvailableGPorts(int portCount) {
    gPortsAvailable_.store(portCount);
  }
  int getAvailableGPorts() {
    return gPortsAvailable_.load();
  }
  int getPolicerId() {
    return policerId_;
  }
  int readScanIntervalInUsecs();

 private:
  // setup communications between QCM, Switch CPU
  void initRxQueue();
  void initPipeMode();
  void initQcmFirmware();
  void initAvailableGPorts();

  // setup QCM flowtracker
  void initExactMatchGroupCreate();
  int createFlowGroup();
  void createIfpGroup();
  void setFlowViewCfg();
  void setFlowGroupTrigger();
  void setAgingInterval(const int agingIntervalInMsecs);
  int createIfpEntry(int port, bool attachStats);
  void setTrackingParams();

  // port monitoring
  QosQueueIds getQosQueueIds(
      const std::shared_ptr<QcmCfg>& qcmCfg,
      const int portId);
  void findPortsForMonitoring(
      const std::shared_ptr<SwitchState>& swState,
      std::set<bcm_port_t>& ports);
  void setupPortsForMonitoring(const Port2QosQueueIdMap& portMap);
  void setupConfiguredPortsForMonitoring(
      const std::shared_ptr<SwitchState>& swState,
      const std::vector<int32_t>& qcmPortList);

  // destroy routines
  void deleteIfpEntries();
  void destroyFlowGroup();
  void destroyExactMatchGroup();
  void resetBurstMonitor();
  void stopQcmFirmware();
  void destroyPolicer();

  // helper routines
  void updateQcmMonitoredPorts(const Port2QosQueueIdMap& portMap);
  void getPortsForQcmMonitoring(std::set<bcm_port_t>& upPortSet);
  void createAndAttachStats(const int ifpEntry);
  void createPolicer(const int policerRate);
  uint64_t getIfpStatCounter(const int portId, bcm_field_stat_t statType);

  BcmSwitch* hw_;
  // pipe to field group
  std::map<int, bcm_field_group_t> exactMatchGroups_;
  int num_pipes_;

  Port2QosQueueIdMap qcmMonitoredPorts_;
  // map of {port_id,  ifp_entry_id }
  std::map<int, int> portToIfpEntryMap_;
  // map of {ifp_entry, stat_id}
  std::map<int, int> ifpEntryToStatMap_;
  std::mutex monitoredPortsLock_;
  bool qcmInitDone_{false};
  std::shared_ptr<QcmCfg> qcmCfg_;
  std::unique_ptr<BcmQcmCollector> qcmCollector_;
  std::atomic<int> gPortsAvailable_{0};
  bcm_policer_t policerId_{0};
};

} // namespace facebook::fboss
