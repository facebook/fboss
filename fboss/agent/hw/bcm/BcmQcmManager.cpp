/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"

extern "C" {
#include <bcm/collector.h>
#include <bcm/cosq.h>
#include <bcm/flowtracker.h>
#include <bcm/init.h>
#include <bcm/port.h>
#include <bcm/switch.h>
#include <bcm/types.h>
#if (!defined(BCM_VER_MAJOR))
#define sal_memset memset
#define BCM_WARM_BOOT_SUPPORT
#include <soc/opensoc.h>
#else
#include <soc/drv.h>
#endif
}

namespace {
constexpr int kUc0CosQueue = 46;
constexpr int kUc0DmaChannel = 5;
// just use 1 flow group Id for now always
constexpr int kQcmFlowGroupId = 1;
constexpr int kQcmDomainId = 0x123456;

std::vector<bcm_flowtracker_tracking_param_type_t> kBcmFlowTrackerParamsVec = {
    bcmFlowtrackerTrackingParamTypeSrcIPv6,
    bcmFlowtrackerTrackingParamTypeDstIPv6,
    bcmFlowtrackerTrackingParamTypeL4SrcPort,
    bcmFlowtrackerTrackingParamTypeL4DstPort,
    bcmFlowtrackerTrackingParamTypeIPProtocol,
    bcmFlowtrackerTrackingParamTypeIngPort};

} // namespace

namespace facebook::fboss {

void BcmQcmManager::createIfpGroup() {
  bcm_field_qset_t qset;
  /* create IFP group */
  BCM_FIELD_QSET_INIT(qset);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyExactMatchHitStatus);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);

  utility::createFPGroup(
      hw_->getUnit(),
      qset,
      FLAGS_qcm_ifp_gid /* gid */,
      FLAGS_qcm_ifp_pri /* pri*/);
}

void BcmQcmManager::initExactMatchGroupCreate() {
  bcm_field_aset_t actn;
  bcm_field_qset_t qset;
  bcm_field_group_config_t gcfg;
  bcm_port_config_t port_config;

  BCM_FIELD_QSET_INIT(qset);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngressExactMatch);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);

  BCM_FIELD_ASET_INIT(actn);
  BCM_FIELD_ASET_ADD(actn, bcmFieldActionStatGroup);

  bcm_port_config_t_init(&port_config);
  auto rv = bcm_port_config_get(hw_->getUnit(), &port_config);
  bcmCheckError(rv, "bcm_port_config_get failed");

  for (int pipe = 0; pipe < num_pipes_; pipe++) {
    bcm_field_group_config_t_init(&gcfg);
    gcfg.qset = qset;
    gcfg.aset = actn;
    gcfg.priority = FLAGS_qcm_ifp_pri;
    gcfg.mode = bcmFieldGroupModeDouble;

    gcfg.flags = BCM_FIELD_GROUP_CREATE_WITH_MODE |
        BCM_FIELD_GROUP_CREATE_WITH_ASET | BCM_FIELD_GROUP_CREATE_WITH_PORT;
    if (BCM_PBMP_IS_NULL(port_config.per_pipe[pipe])) {
      /* No ports in this pipe */
      XLOG(WARN) << "Pipe " << pipe << "doesn't have any port configured";
      continue;
    } else {
      BCM_PBMP_ASSIGN(gcfg.ports, port_config.per_pipe[pipe]);
    }

    rv = bcm_field_group_config_create(hw_->getUnit(), &gcfg);
    bcmCheckError(rv, "bcm_field_group_config_create failed");

    exactMatchGroups_[pipe] = gcfg.group;
    XLOG(INFO) << "Em groups pipe " << pipe
               << " populated with group: " << gcfg.group;
  }
}

// TOOD: This goes away once, we use commom ACL infra to create
// acls in other groups. Used for tests
uint64_t BcmQcmManager::getIfpStatCounter(const int portId) {
  std::lock_guard<std::mutex> g(monitoredPortsLock_);
  uint64_t value = 0;
  auto iter = portToIfpEntryMap_.find(portId);
  if (iter == portToIfpEntryMap_.end()) {
    XLOG(ERR) << "Unable to find ifp entry for port: " << portId;
    return 0;
  }

  int ifpEntry = iter->second;
  auto statIdIter = ifpEntryToStatMap_.find(ifpEntry);
  if (statIdIter == ifpEntryToStatMap_.end()) {
    XLOG(ERR) << "Unable to find the stat counter attached to ifp entry : "
              << ifpEntry;
  }

  bcm_field_stat_t type = bcmFieldStatPackets;
  auto rv =
      bcm_field_stat_get(hw_->getUnit(), statIdIter->second, type, &value);
  bcmCheckError(rv, "Stat get failed");
  return value;
}

// TODO: This routine goes away once we use common ACL infra to create
// acls
void BcmQcmManager::createAndAttachStats(const int ifpEntry) {
  bcm_field_stat_t type = bcmFieldStatPackets;
  int stat_id = 0;
  auto rv = bcm_field_stat_create(
      hw_->getUnit(), FLAGS_qcm_ifp_gid, 1, &type, &stat_id);
  bcmCheckError(rv, "Failed to install stats");

  rv = bcm_field_entry_stat_attach(hw_->getUnit(), ifpEntry, stat_id);
  bcmCheckError(rv, "Statistics attach failed");

  ifpEntryToStatMap_[ifpEntry] = stat_id;
  XLOG(INFO) << "ifpEntry=" << ifpEntry << ", stat_id=" << stat_id;
}

void BcmQcmManager::updateQcmMonitoredPortsIfNeeded(
    const std::set<bcm_port_t>& updatedPorts,
    bool installStats) {
  std::lock_guard<std::mutex> g(monitoredPortsLock_);
  int ifpEntry = 0;
  for (const auto& updatedPort : updatedPorts) {
    if (qcmMonitoredPorts_.find(updatedPort) == qcmMonitoredPorts_.end()) {
      XLOG(INFO) << "Create IFP entry for: " << updatedPort
                 << ", installStats: " << installStats;
      ifpEntry =
          createIfpEntry(updatedPort, false /* create policer */, installStats);
      portToIfpEntryMap_[updatedPort] = ifpEntry;
    }
  }

  if (updatedPorts != qcmMonitoredPorts_) {
    qcmMonitoredPorts_ = updatedPorts;
    updateQcmMonitoredPorts(qcmMonitoredPorts_);
  }
}

BcmQcmManager::~BcmQcmManager() {
  XLOG(INFO) << "Destroying Qcm Manager";
  stopQcm();
}

void BcmQcmManager::stopQcm() {
  if (!qcmInitDone_) {
    return;
  }
  XLOG(INFO) << "Stopping QCM ..";
  // TODO rohitpuri
  qcmInitDone_ = false;
}

int BcmQcmManager::createFlowGroup() {
  bcm_flowtracker_group_info_t flow_group_info;
  int flow_group_id = kQcmFlowGroupId;
  uint32 options = 0;

  options = BCM_FLOWTRACKER_GROUP_WITH_ID;
  bcm_flowtracker_group_info_t_init(&flow_group_info);
  flow_group_info.observation_domain_id = kQcmDomainId;
  for (int i = 0; i < num_pipes_; i++) {
    if (exactMatchGroups_.find(i) == exactMatchGroups_.end()) {
      XLOG(WARN) << "Unable to find the exact match group for pipe: " << i;
      continue;
    }
    flow_group_info.field_group[i] = exactMatchGroups_[i];
  }

  auto rv = bcm_flowtracker_group_create(
      hw_->getUnit(), options, &flow_group_id, &flow_group_info);
  bcmCheckError(rv, "Failed in bcm_flowtracker_group_create");

  XLOG(INFO) << "Flow group created for gruop_id: " << flow_group_id;
  return BCM_E_NONE;
}

void BcmQcmManager::flowLimitSet(int flowLimit) {
  auto rv = bcm_flowtracker_group_flow_limit_set(
      hw_->getUnit(), kQcmFlowGroupId /* flow_group_id */, flowLimit);
  bcmCheckError(rv, "bcm_flowtracker_group_flow_limit_set failed");
  XLOG(INFO, "bcmFlowLimitSet done");
}

int BcmQcmManager::createIfpEntry(int port, bool usePolicer, bool createStats) {
  int rv = 0;

  bcm_field_entry_t entry;
  rv = bcm_field_entry_create(hw_->getUnit(), FLAGS_qcm_ifp_gid, &entry);
  bcmCheckError(rv, "failed to create fp entry");

  rv = bcm_field_qualify_SrcPort(
      hw_->getUnit(), entry, 0x0, 0xff, (bcm_port_t)port, 0xff);
  bcmCheckError(rv, "failed to create qualifier for srcPort");

  rv = bcm_field_qualify_ExactMatchHitStatus(
      hw_->getUnit(), entry, FLAGS_qcm_ifp_pri, 0, 0xFF);
  bcmCheckError(rv, "failed to create qualifier for ExactMatchHitStatus");

  if (usePolicer) {
    rv = bcm_field_entry_policer_attach(hw_->getUnit(), entry, 0, 0);
    bcmCheckError(rv, "bcm_field_entry_policer_attach failed");

    rv = bcm_field_action_add(
        hw_->getUnit(), entry, bcmFieldActionGpCopyToCpu, 1, 1);
    bcmCheckError(rv, "bcm_field_action_add failed");
  } else {
    rv = bcm_field_action_add(
        hw_->getUnit(), entry, bcmFieldActionCopyToCpu, 1, 1 /*flowGroupId*/);
    bcmCheckError(rv, "failed to create action");
  }

  rv = bcm_field_action_add(
      hw_->getUnit(), entry, bcmFieldActionCosQCpuNew, 46, 0);
  bcmCheckError(rv, "failed to create action");

  XLOG(INFO) << "createStats:" << createStats;
  if (createStats) {
    createAndAttachStats(entry);
  }

  // Install field processor entry
  bcm_field_entry_install(hw_->getUnit(), entry);
  bcmCheckError(rv, "Failed to install link-local mcast field processor rule");

  XLOG(INFO) << "IFP entry installed for port: " << port;
  return entry;
}

void BcmQcmManager::setAgingInterval(const int agingIntervalInMsecs) {
  auto rv = bcm_flowtracker_group_age_timer_set(
      hw_->getUnit(), kQcmFlowGroupId, agingIntervalInMsecs);
  bcmCheckError(rv, "bcm_flowtracker_group_age_timer_set failed");
}

void BcmQcmManager::setFlowGroupTrigger() {
  bcm_flowtracker_export_trigger_info_t trigger;

  sal_memset(&trigger, 0, sizeof(trigger));
  BCM_FLOWTRACKER_TRIGGER_CLEAR_ALL(trigger);
  BCM_FLOWTRACKER_TRIGGER_SET(trigger, bcmFlowtrackerExportTriggerCongestion);

  auto rv = bcm_flowtracker_group_export_trigger_set(
      hw_->getUnit(), kQcmFlowGroupId, &trigger);
  bcmCheckError(rv, "bcm_flowtracker_group_export_trigger_set failed");
}

void BcmQcmManager::setTrackingParams() {
  bcm_flowtracker_tracking_param_info_t
      trackingParamsArray[kBcmFlowTrackerParamsVec.size()];
  int index = 0;
  for (const auto& bcmFlowTrackerElem : kBcmFlowTrackerParamsVec) {
    bcm_flowtracker_tracking_param_info_t trackingParams;
    bcm_flowtracker_tracking_param_info_t_init(&trackingParams);
    trackingParams.param = bcmFlowTrackerElem;
    trackingParamsArray[index++] = trackingParams;
  }

  auto rv = bcm_flowtracker_group_tracking_params_set(
      hw_->getUnit(),
      kQcmFlowGroupId /* flow_group_id */,
      kBcmFlowTrackerParamsVec.size(),
      trackingParamsArray);
  bcmCheckError(rv, "bcm_flowtracker_group_tracking_params_set failed");
  XLOG(INFO) << "bcmSetTrackingParams done, rv " << rv;
}

uint32_t BcmQcmManager::getLearnedFlowCount() {
  uint32_t flowCount;
  auto rv = bcm_flowtracker_group_flow_count_get(
      hw_->getUnit(), kQcmFlowGroupId, &flowCount);
  bcmCheckError(rv, "bcm_flowtracker_group_get failed");
  return flowCount;
}

void BcmQcmManager::initQcm() {
  XLOG(INFO) << "Start QCM";

  bcm_info_t info;
  auto rv = bcm_info_get(hw_->getUnit(), &info);
  bcmCheckError(rv, "failed to get unit info");

  num_pipes_ = info.num_pipes;

  rv = bcm_rx_queue_channel_set(hw_->getUnit(), kUc0CosQueue, kUc0DmaChannel);
  bcmCheckError(rv, "bcm_rx_queue_channel_set failed");

  rv = bcm_field_group_oper_mode_set(
      hw_->getUnit(),
      bcmFieldQualifyStageIngressExactMatch,
      bcmFieldGroupOperModePipeLocal);
  bcmCheckError(rv, "bcm_field_group_oper_mode_set failed");

  initExactMatchGroupCreate();
  createIfpGroup();

  createFlowGroup();

  setFlowGroupTrigger();
  setAgingInterval(qcmCfg_.agingIntervalInMsecs);

  setTrackingParams();

  /* create flow group */
  flowLimitSet(qcmCfg_.flowLimit);
  qcmInitDone_ = true;
  XLOG(INFO) << "Qcm init completed";
}
} // namespace facebook::fboss
