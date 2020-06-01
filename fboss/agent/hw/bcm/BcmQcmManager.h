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

class BcmQcmManager {
 public:
  explicit BcmQcmManager(BcmSwitch* hw) : hw_(hw) {}
  ~BcmQcmManager();
  void initQcm();
  void stopQcm();
  void updateQcmMonitoredPortsIfNeeded(
      const std::set<bcm_port_t>& upPorts,
      bool installStats = false);
  int createIfpEntry(int port, bool usePolicer, bool attachStats);
  uint64_t getIfpStatCounter(const int portId);
  uint32_t getLearnedFlowCount();
  void flowLimitSet(int flowLimit);

 private:
  void updateQcmMonitoredPorts(std::set<bcm_port_t>& upPorts);
  void initExactMatchGroupCreate();
  void getPortsForQcmMonitoring(std::set<bcm_port_t>& upPortSet);
  int createFlowGroup();
  void createIfpGroup();
  void setFlowViewCfg();
  void createAndAttachStats(const int ifpEntry);
  void setFlowGroupTrigger();
  void setAgingInterval(const int agingIntervalInMsecs);
  void setTrackingParams();

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
  cfg::QcmConfig qcmCfg_;
};

} // namespace facebook::fboss
