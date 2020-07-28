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
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmFwLoader.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmQcmCollector.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/SwitchState.h"

extern "C" {
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

DEFINE_bool(
    enable_qcm_ifp_statistics,
    false,
    "Enable statistics on ifp entries. Used for testing only");
DEFINE_int32(
    init_gport_available_count,
    0,
    "User override for the initial gport count");

namespace {
constexpr int kUc0CosQueue = 46; // cos queue that maps from switch to QCM CPU
constexpr int kUc0DmaChannel = 5; // DMA channel id from bcm switch to QCM CPU
constexpr int kQcmFlowGroupId = 1; // Use a specific flow group Id (out of 15)
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

int BcmQcmManager::getQcmFlowGroupId() {
  return kQcmFlowGroupId;
}

void BcmQcmManager::stopQcmFirmware() {
  XLOG(INFO) << "[QCM] Firmware stop ..";
  BcmFwLoader::stopFirmware(hw_);

  auto rv = bcm_cosq_burst_monitor_detach(hw_->getUnit());
  bcmCheckError(rv, "bcm_cosq_burst_monitor_detach failed");
}

void BcmQcmManager::initQcmFirmware() {
  XLOG(INFO) << "[QCM] Firmware load ..";
  BcmFwLoader::loadFirmware(hw_, hw_->getPlatform()->getAsic());

  auto rv = bcm_cosq_burst_monitor_init(hw_->getUnit());
  bcmCheckError(rv, "bcm_cosq_burst_monitor_init failed");
}

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

uint64_t BcmQcmManager::getIfpHitStatCounter(const int portId) {
  return getIfpStatCounter(portId, bcmFieldStatPackets);
}

uint64_t BcmQcmManager::getIfpRedPktStatCounter(const int portId) {
  // return count of pkts marked red
  return getIfpStatCounter(portId, bcmFieldStatRedPackets);
}

// TOOD: This goes away once, we use commom ACL infra to create
// acls in other groups. Used for tests
uint64_t BcmQcmManager::getIfpStatCounter(
    const int portId,
    bcm_field_stat_t statType) {
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
    return 0;
  }

  auto rv =
      bcm_field_stat_get(hw_->getUnit(), statIdIter->second, statType, &value);
  bcmCheckError(rv, "Stat get failed for stat=", statType);
  return value;
}

// TODO: This routine goes away once we use common ACL infra to create
// acls
void BcmQcmManager::createAndAttachStats(const int ifpEntry) {
  std::vector<bcm_field_stat_t> statTypes{bcmFieldStatPackets,
                                          bcmFieldStatRedPackets};
  int stat_id = 0;
  auto rv = bcm_field_stat_create(
      hw_->getUnit(),
      FLAGS_qcm_ifp_gid,
      statTypes.size(),
      statTypes.data(),
      &stat_id);
  bcmCheckError(rv, "Failed to install stats");

  rv = bcm_field_entry_stat_attach(hw_->getUnit(), ifpEntry, stat_id);
  bcmCheckError(rv, "Statistics attach failed");

  ifpEntryToStatMap_[ifpEntry] = stat_id;
  XLOG(INFO) << "ifpEntry=" << ifpEntry << ", stat_id=" << stat_id;
}

void BcmQcmManager::setupPortsForMonitoring(const Port2QosQueueIdMap& portMap) {
  std::lock_guard<std::mutex> g(monitoredPortsLock_);
  int ifpEntry = 0;
  for (const auto& port : portMap) {
    if (qcmMonitoredPorts_.find(port.first) == qcmMonitoredPorts_.end()) {
      XLOG(DBG3) << "Create IFP entry for: " << port.first;
      ifpEntry = createIfpEntry(port.first, FLAGS_enable_qcm_ifp_statistics);
      portToIfpEntryMap_[port.first] = ifpEntry;
      qcmMonitoredPorts_[port.first] = port.second;
    }
  }
  updateQcmMonitoredPorts(qcmMonitoredPorts_);
}

BcmQcmManager::BcmQcmManager(BcmSwitch* hw)
    : hw_(hw), qcmCollector_(new BcmQcmCollector(hw, this)) {}

BcmQcmManager::~BcmQcmManager() {
  XLOG(INFO) << "Destroying Qcm Manager";
  stop();
  // reset collector
  qcmCollector_.reset();
}

void BcmQcmManager::destroyFlowGroup() {
  auto rv = bcm_flowtracker_group_destroy(hw_->getUnit(), kQcmFlowGroupId);
  bcmCheckError(rv, "destroyFlowGroup failed");
}

bool BcmQcmManager::isFlowTrackerDisabled() {
  bcm_flowtracker_group_info_t flow_group_info;
  auto rv = bcm_flowtracker_group_get(
      hw_->getUnit(), kQcmFlowGroupId, &flow_group_info);
  if (rv == BCM_E_INIT) {
    return true;
  }
  return false;
}

void BcmQcmManager::destroyExactMatchGroup() {
  int rv = 0;
  for (const auto exactMatchElem : exactMatchGroups_) {
    rv = bcm_field_group_destroy(hw_->getUnit(), exactMatchElem.second);
    bcmCheckError(rv, "Exact macth group destroy failed");
  }
  exactMatchGroups_.clear();
  rv = bcm_field_group_destroy(hw_->getUnit(), FLAGS_qcm_ifp_gid);
  bcmCheckError(rv, "ifp group destroy failed");
}

void BcmQcmManager::deleteIfpEntries() {
  int rv = 0;
  for (const auto& ifpEntry : portToIfpEntryMap_) {
    if (policerId_) {
      rv = bcm_field_entry_policer_detach(hw_->getUnit(), ifpEntry.second, 0);
      bcmCheckError(rv, "bcm_field_entry_policer_detach failed");
    }
    rv = bcm_field_entry_destroy(hw_->getUnit(), ifpEntry.second);
    bcmCheckError(rv, "bcm_field_entry_destroy failed");
  }
  portToIfpEntryMap_.clear();
  ifpEntryToStatMap_.clear();
  qcmMonitoredPorts_.clear();
}

void BcmQcmManager::destroyPolicer() {
  if (policerId_) {
    auto rv = bcm_policer_destroy(hw_->getUnit(), policerId_);
    bcmCheckError(rv, "bcm_policer_destroy failed");
    policerId_ = 0; // reset
  }
}

void BcmQcmManager::stop() {
  if (!qcmInitDone_) {
    return;
  }
  XLOG(INFO) << "Stopping QCM ..";
  resetBurstMonitor();
  if (qcmCollector_) {
    qcmCollector_->stop();
  }
  deleteIfpEntries(); // step 2
  destroyFlowGroup(); // step 3
  destroyExactMatchGroup(); // step 4
  destroyPolicer();

  // stop firmware
  stopQcmFirmware();
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

int BcmQcmManager::createIfpEntry(int port, bool createStats) {
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

  if (policerId_) {
    rv = bcm_field_entry_policer_attach(hw_->getUnit(), entry, 0, policerId_);
    bcmCheckError(rv, "bcm_field_entry_policer_attach failed");

    rv = bcm_field_action_add(
        hw_->getUnit(), entry, bcmFieldActionGpCopyToCpu, 1, kQcmFlowGroupId);
    bcmCheckError(rv, "bcm_field_action_add failed");
  } else {
    rv = bcm_field_action_add(
        hw_->getUnit(), entry, bcmFieldActionCopyToCpu, 1, kQcmFlowGroupId);
    bcmCheckError(rv, "failed to create action");
  }

  rv = bcm_field_action_add(
      hw_->getUnit(), entry, bcmFieldActionCosQCpuNew, 46, 0);
  bcmCheckError(rv, "failed to create action");
  if (createStats) {
    createAndAttachStats(entry);
  }

  // Install field processor entry
  bcm_field_entry_install(hw_->getUnit(), entry);
  bcmCheckError(rv, "Failed to install link-local mcast field processor rule");

  XLOG(DBG3) << "IFP entry installed for port: " << port;
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

void BcmQcmManager::initRxQueue() {
  auto rv =
      bcm_rx_queue_channel_set(hw_->getUnit(), kUc0CosQueue, kUc0DmaChannel);
  bcmCheckError(rv, "bcm_rx_queue_channel_set failed");
}

void BcmQcmManager::initPipeMode() {
  bcm_info_t info;
  auto rv = bcm_info_get(hw_->getUnit(), &info);
  bcmCheckError(rv, "failed to get unit info");

  num_pipes_ = info.num_pipes;
  rv = bcm_field_group_oper_mode_set(
      hw_->getUnit(),
      bcmFieldQualifyStageIngressExactMatch,
      bcmFieldGroupOperModePipeLocal);
  bcmCheckError(rv, "bcm_field_group_oper_mode_set failed");
}

bool BcmQcmManager::isQcmSupported(BcmSwitch* hw) {
#if BCM_VER_SUPPORT_QCM
  if (hw && hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::QCM)) {
    return true;
  }
  // fall through if BCM_VER_SUPPORT_QCMm but no qcm support
#endif
  return false;
}

QosQueueIds BcmQcmManager::getQosQueueIds(
    const std::shared_ptr<QcmCfg>& qcmCfg,
    const int portId) {
  // QCM queues to be monitored comes from config
  // Today QCM will monitor icp, silver, gold, bronze queues on the port
  // (olympic qos model) or upto first 5 queues if queue_per_host is enabled.
  const auto& port2QosQueueIdMap = qcmCfg->getPort2QosQueueIdMap();
  auto iter = port2QosQueueIdMap.find(portId);
  if (iter == port2QosQueueIdMap.end()) {
    XLOG(ERR) << "Unable to find QosQueueIds for port: " << portId;
    return {};
  }
  return iter->second;
}

// find the candidate ports needed for monitoring
// Note that intent of QCM monitoring is to enable
// monitoring for first x enabled ports
// if these ports later have admin down or are removed
// we don't explicitly remove them from the QCM monitoring
// as this requires removing IFP entries and
// associated state which can be source of problem at
// run-time especially if the port state keeps changing
// So most of this configuration is 1-time programming
// for simpliciity
// @returns true if we end up updating the qcm ports
// @returns false if QCM ports are not updated
bool BcmQcmManager::updateQcmMonitoredPortsIfNeeded(
    const Port2QosQueueIdMap& candidatePortMap) {
  Port2QosQueueIdMap finalPortMap{};
  for (const auto& candidatePort : candidatePortMap) {
    // walk all candidate ports which which satisfy following criteria
    // (1) Enough gports available
    // (2) Not already configured
    const auto& queueSet = candidatePort.second;
    std::lock_guard<std::mutex> g(monitoredPortsLock_);
    if ((qcmMonitoredPorts_.find(candidatePort.first) ==
         qcmMonitoredPorts_.end()) &&
        gPortsAvailable_ >= queueSet.size()) {
      finalPortMap[candidatePort.first] = candidatePort.second;
      gPortsAvailable_ -= queueSet.size();
    }
  }
  if (finalPortMap.size()) {
    setupPortsForMonitoring(finalPortMap);
  }
  return finalPortMap.size() ? true : false;
}

// invoked during normal cfg reload processing
void BcmQcmManager::processPortsForQcm(
    const std::shared_ptr<SwitchState>& swState) {
  if (!isQcmSupported(hw_)) {
    return;
  }
  if (qcmCfg_->getMonitorQcmCfgPortsOnly()) {
    XLOG(DBG3) << "Skip programming ports which are not in monitorQcmPortList";
    return;
  }
  Port2QosQueueIdMap candidatePortMap{};
  for (const auto& portIDAndBcmPort : *hw_->getPortTable()) {
    PortID portId = portIDAndBcmPort.first;
    // walk all ports which which satify following criteria
    // (1) enabled (admin up)
    if (hw_->isPortEnabled(portId)) {
      const auto& queueIdSet = getQosQueueIds(swState->getQcmCfg(), portId);
      if (!queueIdSet.empty()) {
        candidatePortMap[portId] = queueIdSet;
      }
    }
  }
  updateQcmMonitoredPortsIfNeeded(candidatePortMap);
}

void BcmQcmManager::processQcmConfigChanged(
    const std::shared_ptr<SwitchState> newSwState,
    const std::shared_ptr<SwitchState> oldSwState) {
  if (*newSwState->getQcmCfg() != *oldSwState->getQcmCfg()) {
    // TODO reconfigure the QCM parameters in hardware
    XLOG(WARN) << "QCM on the fly config changes are not supported yet";
    return;
  }
}

// Invoked during qcm init, so these ports get priority
// in programming (as qcm ports are limited)
void BcmQcmManager::setupConfiguredPortsForMonitoring(
    const std::shared_ptr<SwitchState>& swState,
    const std::vector<int32_t>& qcmPortList) {
  Port2QosQueueIdMap candidatePortMap{};
  for (const auto& port : qcmPortList) {
    candidatePortMap[port] = getQosQueueIds(swState->getQcmCfg(), port);
  }
  updateQcmMonitoredPortsIfNeeded(candidatePortMap);
}

void BcmQcmManager::createPolicer(const int policerRate) {
  bcm_policer_config_t policer_config;
  bcm_policer_config_t_init(&policer_config);
  policer_config.flags = BCM_POLICER_MODE_PACKETS;
  policer_config.mode = bcmPolicerModeCommitted;
  policer_config.ckbits_sec = policerRate;
  policer_config.ckbits_burst = 1;

  policerId_ = 0;
  auto rv = bcm_policer_create(hw_->getUnit(), &policer_config, &policerId_);
  bcmCheckError(rv, "bcm_policer_create failed");
  XLOG(DBG3) << "Policer configured with id" << policerId_;
}

int BcmQcmManager::readScanIntervalInUsecs() {
  bcm_cosq_burst_monitor_flow_view_info_t viewCfg;
  auto rv =
      bcm_cosq_burst_monitor_flow_view_config_get(hw_->getUnit(), &viewCfg);
  bcmCheckError(rv, "bcm_cosq_burst_monitor_flow_view_config_get failed");
  return viewCfg.scan_interval_usecs;
}

void BcmQcmManager::init(const std::shared_ptr<SwitchState>& swState) {
  if (!isQcmSupported(hw_)) {
    return;
  }
  XLOG(INFO) << "[QCM] Start Init";

  qcmCfg_ = swState->getQcmCfg();
  initQcmFirmware();
  initRxQueue();
  initPipeMode();
  if (FLAGS_init_gport_available_count) {
    // user override, used in tests to limit
    // number of configured ports.
    gPortsAvailable_ = FLAGS_init_gport_available_count;
  } else {
    // pick from the HW limits
    initAvailableGPorts();
  }

  initExactMatchGroupCreate();
  if (qcmCfg_->getPpsToQcm().has_value()) {
    createPolicer(qcmCfg_->getPpsToQcm().value());
  }

  createIfpGroup();

  /* program flow tracker specific tables */
  createFlowGroup();
  setFlowGroupTrigger();
  setAgingInterval(qcmCfg_->getAgingInterval());
  setTrackingParams();
  flowLimitSet(qcmCfg_->getFlowLimit());
  setFlowViewCfg();

  // Configured ports get priority in monitoring
  // these are the uplnk ports today, but can be configured differently
  setupConfiguredPortsForMonitoring(swState, qcmCfg_->getMonitorQcmPortList());
  /* create collector */
  qcmCollector_->init(swState);

  qcmInitDone_ = true;
  XLOG(INFO) << "[QCM] Init done";
}
} // namespace facebook::fboss
