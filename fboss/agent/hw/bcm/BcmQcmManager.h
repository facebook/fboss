// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/types.h"

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

namespace facebook::fboss {

class BcmQcmCollector;
class QcmCfg;

class BcmQcmManager {
 public:
  explicit BcmQcmManager(BcmSwitch* hw);
  ~BcmQcmManager();
  void init(const std::shared_ptr<SwitchState>& swState);
  void stop();
  void updateQcmMonitoredPortsIfNeeded(
      const std::set<bcm_port_t>& upPorts,
      bool installStats = false);
  void flowLimitSet(int flowLimit);

  // utility routines
  static bool isQcmSupported(BcmSwitch* hw);

  // utility routines used by tests
  uint64_t getIfpStatCounter(const int portId);
  uint32_t getLearnedFlowCount();
  static int getQcmFlowGroupId();
  bool isFlowTrackerDisabled();

 private:
  // setup communications between QCM, Switch CPU
  void initRxQueue();
  void initPipeMode();
  void initQcmFirmware();

  // setup QCM flowtracker
  void initExactMatchGroupCreate();
  int createFlowGroup();
  void createIfpGroup();
  void setFlowViewCfg();
  void setFlowGroupTrigger();
  void setAgingInterval(const int agingIntervalInMsecs);
  int createIfpEntry(int port, bool usePolicer, bool attachStats);
  void setTrackingParams();

  // destroy routines
  void deleteIfpEntries();
  void destroyFlowGroup();
  void destroyExactMatchGroup();
  void resetBurstMonitor();
  void stopQcmFirmware();

  // helper routines
  void updateQcmMonitoredPorts(std::set<bcm_port_t>& upPorts);
  void getPortsForQcmMonitoring(std::set<bcm_port_t>& upPortSet);
  void createAndAttachStats(const int ifpEntry);

  BcmSwitch* hw_;
  // pipe to field group
  std::map<int, bcm_field_group_t> exactMatchGroups_;
  int num_pipes_;
  std::set<bcm_port_t> qcmMonitoredPorts_;
  // map of {port_id,  ifp_entry_id }
  std::map<int, int> portToIfpEntryMap_;
  // map of {ifp_entry, stat_id}
  std::map<int, int> ifpEntryToStatMap_;
  std::mutex monitoredPortsLock_;
  bool qcmInitDone_{false};
  std::shared_ptr<QcmCfg> qcmCfg_;
  std::unique_ptr<BcmQcmCollector> qcmCollector_;
};

} // namespace facebook::fboss
