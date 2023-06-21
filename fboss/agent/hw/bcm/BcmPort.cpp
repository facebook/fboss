/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPort.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/bimap.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <map>

#include <fb303/ServiceData.h>
#include <folly/Conv.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/hw/CounterUtils.h"

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmPortIngressBufferManager.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmPortResourceBuilder.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/BcmPrbs.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/PhyUtils.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/init.h>
#include <bcm/link.h>
#include <bcm/port.h>
#include <bcm/qos.h>
#include <bcm/stat.h>
#include <bcm/types.h>
#if (defined(IS_OPENNSA))
#include <soc/opensoc.h>
#else
#include <soc/drv.h>
#endif
}

using std::shared_ptr;
using std::string;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

using facebook::stats::MonotonicCounter;

namespace {
constexpr int kPfcDeadlockDetectionTimeMsecMax = 1500;
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
/**
 * Size of a single page of codeword errors from bcm_port_fdr_stats_get()
 *
 * The struct bcm_port_fdr_stats_t actually have 9 entries in it with
 * comments saying that some chips have 9. But the API docs from Broadcom
 * says that no chip does that as of Tomahawk 4. It's not clear what chip
 * and under what FEC mode will we see 9. Revisit if this changes in the
 * future.
 */
static constexpr int kCodewordErrorsPageSize = 8;
#endif

bool hasPortQueueChanges(
    const shared_ptr<facebook::fboss::Port>& oldPort,
    const shared_ptr<facebook::fboss::Port>& newPort) {
  if (oldPort->getPortQueues()->size() != newPort->getPortQueues()->size()) {
    return true;
  }

  for (const auto& newQueue : std::as_const(*(newPort->getPortQueues()))) {
    auto oldQueue = oldPort->getPortQueues()->impl().at(newQueue->getID());
    if (oldQueue->getName() != newQueue->getName()) {
      return true;
    }
  }

  return false;
}

bool hasPfcStatusChangedToEnabled(
    const shared_ptr<facebook::fboss::Port>& oldPort,
    const shared_ptr<facebook::fboss::Port>& newPort) {
  if (newPort->getPfc().has_value() && !oldPort->getPfc().has_value()) {
    return true;
  }
  return false;
}

bool hasPfcStatusChangedToDisabled(
    const shared_ptr<facebook::fboss::Port>& oldPort,
    const shared_ptr<facebook::fboss::Port>& newPort) {
  if (oldPort->getPfc().has_value() && !newPort->getPfc().has_value()) {
    return true;
  }
  return false;
}

using namespace facebook::fboss;

bcm_port_resource_t getCurrentPortResource(int unit, bcm_gport_t gport) {
  bcm_port_resource_t portResource;
  bcm_port_resource_t_init(&portResource);
  auto rv = bcm_port_resource_speed_get(unit, gport, &portResource);
  bcmCheckError(rv, "failed to get port resource on port ", gport);
  XLOG(DBG4) << "Get current port resorce: port=" << portResource.port
             << ", physical_port=" << portResource.physical_port
             << ", speed=" << portResource.speed
             << ", fec_type=" << portResource.fec_type << ", phy_lane_config=0x"
             << std::hex << portResource.phy_lane_config
             << ", link_training=" << std::dec << portResource.link_training;
  return portResource;
}

void fec_stat_accumulate(
    int unit,
    bcm_port_t port,
    bcm_port_phy_control_t type,
    int64_t* accumulator) {
  int rv;
  uint32 count;

  rv = bcm_port_phy_control_get(unit, port, type, &count);
  if (BCM_FAILURE(rv)) {
    return;
  }

  if (*accumulator == hardware_stats_constants::STAT_UNINITIALIZED()) {
    *accumulator = 0;
  }

  COMPILER_64_ADD_32(*accumulator, count);
}

static void fdrStatConfigure(
    __attribute__((unused)) int unit,
    __attribute__((unused)) bcm_port_t port,
    __attribute__((unused)) bool enable,
    __attribute__((unused)) int symbErrSel = 0) {
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  // See Broadcom case for PDF doc of FDR APIs: 12196665
  //
  // Normally, FEC corrects errors and counts code words it cannot correct.
  // With FDR, we can see corrected errors so we can act pre-emptively when
  // they are getting close to uncorrectable. This requires SDK 6.5.21+.
  //
  // It has 2 modes. 1: Raise interrupt and call our code when too many
  // errors are seen in a time window. 2: We poll counters periodically. We
  // currently don't have any use case for #1. So we'll use #2 and turn off
  // #1. (But SDK does allow both to be active at the same time if say we
  // want to pre-emptively reset the port in the future.)
  //
  // To use #2, we just need to enable FDR and set the page of symbol errors
  // to monitor. Each page contains 8 counters. If FEC544 is in use, we have
  // 16 counters in 2 pages. All other FEC modes have a single page of 8
  // counters.
  bcm_port_fdr_config_t fdr_config, current_fdr_config;

  // The init API for this struct is missing in OpenNSA.
  // Broadcom case CS00012208804
  // HACK: memset it to zero manually.
#if (defined(IS_OPENNSA))
  memset(&fdr_config, 0, sizeof(bcm_port_fdr_config_t));
#else
  bcm_port_fdr_config_t_init(&fdr_config);
#endif
  fdr_config.fdr_enable = enable;
  DCHECK(symbErrSel >= 0 && symbErrSel <= 1);
  fdr_config.symb_err_stats_sel = symbErrSel;

  bcmCheckError(
      bcm_port_fdr_config_get(unit, port, &current_fdr_config),
      "Failed to fetch current error telemetry config for unit ",
      unit,
      " port ",
      port);

  if (memcmp(&fdr_config, &current_fdr_config, sizeof(bcm_port_fdr_config_t)) ==
      0) {
    return;
  }

  bcmCheckError(
      bcm_port_fdr_config_set(unit, port, &fdr_config),
      "Failed to configure port error telemetry on unit ",
      unit,
      " port ",
      port);

  if (enable) {
    // Clear out any left over stats that may be for a different page
    bcm_port_fdr_stats_t fdr_stats;
    bcm_port_fdr_stats_get(unit, port, &fdr_stats);
  }
#endif
}

static const std::string getFdrStatsKey(int errorsPerCodeword) {
  return folly::to<std::string>(kErrorsPerCodeword(), ".", errorsPerCodeword);
}

} // namespace

namespace facebook::fboss {

static const std::vector<bcm_stat_val_t> kInPktLengthStats = {
    snmpBcmReceivedPkts64Octets,
    snmpBcmReceivedPkts65to127Octets,
    snmpBcmReceivedPkts128to255Octets,
    snmpBcmReceivedPkts256to511Octets,
    snmpBcmReceivedPkts512to1023Octets,
    snmpBcmReceivedPkts1024to1518Octets,
    snmpBcmReceivedPkts1519to2047Octets,
    snmpBcmReceivedPkts2048to4095Octets,
    snmpBcmReceivedPkts4095to9216Octets,
    snmpBcmReceivedPkts9217to16383Octets,
};
static const std::vector<bcm_stat_val_t> kOutPktLengthStats = {
    snmpBcmTransmittedPkts64Octets,
    snmpBcmTransmittedPkts65to127Octets,
    snmpBcmTransmittedPkts128to255Octets,
    snmpBcmTransmittedPkts256to511Octets,
    snmpBcmTransmittedPkts512to1023Octets,
    snmpBcmTransmittedPkts1024to1518Octets,
    snmpBcmTransmittedPkts1519to2047Octets,
    snmpBcmTransmittedPkts2048to4095Octets,
    snmpBcmTransmittedPkts4095to9216Octets,
    snmpBcmTransmittedPkts9217to16383Octets,
};
static const std::vector<bcm_stat_val_t> kInPfcXonStats = {
    snmpBcmRxPFCFrameXonPriority0,
    snmpBcmRxPFCFrameXonPriority1,
    snmpBcmRxPFCFrameXonPriority2,
    snmpBcmRxPFCFrameXonPriority3,
    snmpBcmRxPFCFrameXonPriority4,
    snmpBcmRxPFCFrameXonPriority5,
    snmpBcmRxPFCFrameXonPriority6,
    snmpBcmRxPFCFrameXonPriority7,
};
static const std::vector<bcm_stat_val_t> kInPfcStats = {
    snmpBcmRxPFCFramePriority0,
    snmpBcmRxPFCFramePriority1,
    snmpBcmRxPFCFramePriority2,
    snmpBcmRxPFCFramePriority3,
    snmpBcmRxPFCFramePriority4,
    snmpBcmRxPFCFramePriority5,
    snmpBcmRxPFCFramePriority6,
    snmpBcmRxPFCFramePriority7,
};
static const std::vector<bcm_stat_val_t> kOutPfcStats = {
    snmpBcmTxPFCFramePriority0,
    snmpBcmTxPFCFramePriority1,
    snmpBcmTxPFCFramePriority2,
    snmpBcmTxPFCFramePriority3,
    snmpBcmTxPFCFramePriority4,
    snmpBcmTxPFCFramePriority5,
    snmpBcmTxPFCFramePriority6,
    snmpBcmTxPFCFramePriority7,
};

MonotonicCounter* BcmPort::getPortCounterIf(folly::StringPiece statKey) {
  auto pcitr = portCounters_.find(statKey.str());
  return pcitr != portCounters_.end() ? &pcitr->second : nullptr;
}

void BcmPort::reinitPortStat(
    folly::StringPiece statKey,
    folly::StringPiece portName) {
  auto stat = getPortCounterIf(statKey);

  if (!stat) {
    portCounters_.emplace(
        statKey.str(),
        MonotonicCounter(statName(statKey, portName), fb303::SUM, fb303::RATE));
  } else if (stat->getName() != statName(statKey, portName)) {
    MonotonicCounter newStat{
        statName(statKey, portName), fb303::SUM, fb303::RATE};
    stat->swap(newStat);
    utility::deleteCounter(newStat.getName());
  }
}

void BcmPort::removePortStat(folly::StringPiece statKey) {
  auto stat = getPortCounterIf(statKey);
  if (stat) {
    utility::deleteCounter(stat->getName());
    portCounters_.erase(stat->getName());
  }
}

void BcmPort::removePortPfcStatsLocked(
    const BcmPort::PortStatsWLockedPtr& /* lockedPortStatsPtr */,
    const std::shared_ptr<Port>& swPort,
    const std::vector<PfcPriority>& priorities) {
  XLOG(DBG3) << "Destroy PFC stats for " << swPort->getName();

  // Destroy per priority PFC statistics
  for (auto priority : priorities) {
    removePortStat(getPfcPriorityStatsKey(kInPfc(), priority));
    removePortStat(getPfcPriorityStatsKey(kInPfcXon(), priority));
    removePortStat(getPfcPriorityStatsKey(kOutPfc(), priority));
  }

  // Destroy per port PFC statistics
  removePortStat(kInPfc());
  removePortStat(kOutPfc());
}

void BcmPort::reinitPortPfcStats(const std::shared_ptr<Port>& swPort) {
  auto& portName = swPort->getName();

  XLOG(DBG3) << "Reinitializing PFC stats for " << portName;
  // Reinit per priority PFC statistics
  for (auto priority : swPort->getPfcPriorities()) {
    reinitPortStat(getPfcPriorityStatsKey(kInPfc(), priority), portName);
    reinitPortStat(getPfcPriorityStatsKey(kInPfcXon(), priority), portName);
    reinitPortStat(getPfcPriorityStatsKey(kOutPfc(), priority), portName);
  }

  // Reinit per port PFC statistics
  reinitPortStat(kInPfc(), portName);
  reinitPortStat(kOutPfc(), portName);
}

void BcmPort::reinitPortStatsLocked(
    const BcmPort::PortStatsWLockedPtr& lockedPortStatsPtr,
    const std::shared_ptr<Port>& swPort) {
  auto& portName = swPort->getName();
  XLOG(DBG2) << "Reinitializing stats for " << portName;

  reinitPortStat(kInBytes(), portName);
  reinitPortStat(kInUnicastPkts(), portName);
  reinitPortStat(kInMulticastPkts(), portName);
  reinitPortStat(kInBroadcastPkts(), portName);
  reinitPortStat(kInDiscardsRaw(), portName);
  reinitPortStat(kInDiscards(), portName);
  reinitPortStat(kInErrors(), portName);
  reinitPortStat(kInPause(), portName);
  reinitPortStat(kInIpv4HdrErrors(), portName);
  reinitPortStat(kInIpv6HdrErrors(), portName);
  reinitPortStat(kInDstNullDiscards(), portName);

  reinitPortStat(kOutBytes(), portName);
  reinitPortStat(kOutUnicastPkts(), portName);
  reinitPortStat(kOutMulticastPkts(), portName);
  reinitPortStat(kOutBroadcastPkts(), portName);
  reinitPortStat(kOutDiscards(), portName);
  reinitPortStat(kOutErrors(), portName);
  reinitPortStat(kOutPause(), portName);
  reinitPortStat(kOutEcnCounter(), portName);
  reinitPortStat(kWredDroppedPackets(), portName);
  reinitPortStat(kFecCorrectable(), portName);
  reinitPortStat(kFecUncorrectable(), portName);
  reinitPortStat(kInCongestionDiscards(), portName);

  if (swPort->getPfc().has_value()) {
    // Init port PFC stats only if PFC is enabled on the port!
    reinitPortPfcStats(swPort);
  }

  if (swPort) {
    queueManager_->setPortName(portName);
    queueManager_->setupQueueCounters(swPort->getPortQueues()->impl());
  }

  // (re) init out queue length
  auto statMap = fb303::fbData->getStatMap();
  const auto expType = fb303::AVG;
  outQueueLen_ = statMap->getLockableStat(
      statName("out_queue_length", portName), &expType);
  // (re) init histograms
  auto histMap = fb303::fbData->getHistogramMap();
  fb303::ExportedHistogram pktLenHist(1, 0, kInPktLengthStats.size());
  inPktLengths_ = histMap->getOrCreateLockableHistogram(
      statName("in_pkt_lengths", portName), &pktLenHist);
  outPktLengths_ = histMap->getOrCreateLockableHistogram(
      statName("out_pkt_lengths", portName), &pktLenHist);

  *lockedPortStatsPtr =
      BcmPortStats(queueManager_->getNumQueues(cfg::StreamType::UNICAST));
}

void BcmPort::reinitPortFdrStats(const std::shared_ptr<Port>& swPort) {
  auto& portName = swPort->getName();
  auto fecMaxErrors = getFecMaxErrors();

  for (int i = 0; i < fecMaxErrors; ++i) {
    if (i < fdrStats_.size()) {
      // Idempotently set stat name again, in case port name changed
      fdrStats_[i].setName(statName(getFdrStatsKey(i), portName));
    } else {
      fdrStats_.emplace_back(
          statName(getFdrStatsKey(i), portName),
          fb303::ExportTypeConsts::kSumRate);
    }
  }

  // In case max errors got reduced due to FEC mode changes
  while (fdrStats_.size() > fecMaxErrors) {
    fdrStats_.pop_back();
  }
}

BcmPort::BcmPort(BcmSwitch* hw, bcm_port_t port, BcmPlatformPort* platformPort)
    : hw_(hw), port_(port), platformPort_(platformPort), unit_(hw->getUnit()) {
  // Obtain the gport handle from the port handle.
  int rv = bcm_port_gport_get(unit_, port_, &gport_);
  bcmCheckError(rv, "Failed to get gport for BCM port ", port_);

  queueManager_ =
      std::make_unique<BcmPortQueueManager>(hw_, getPortName(), gport_);
  ingressBufferManager_ =
      std::make_unique<BcmPortIngressBufferManager>(hw_, getPortName(), gport_);

  pipe_ = determinePipe();

  XLOG(DBG2) << "created BCM port:" << port_ << ", gport:" << gport_
             << ", FBOSS PortID:" << platformPort_->getPortID();
}

BcmPort::~BcmPort() {
  destroy();
}

void BcmPort::init(bool warmBoot) {
  if (!warmBoot) {
    // In open source code, we don't have any guarantees for the
    // state of the port at startup. Bringing them down guarantees
    // that things are in a known state.
    //
    // We should only be doing this on cold boot, since warm booting
    // should be initializing the state for us.
    auto rv = bcm_port_enable_set(unit_, port_, false);
    bcmCheckError(rv, "failed to set port to known state: ", port_);
  } else {
    // During warmboot, we need to make sure the enabled ports have
    // EgressQueue FlexCounter attached if needed
    if (auto* flexCounterMgr = hw_->getBcmEgressQueueFlexCounterManager();
        flexCounterMgr && isEnabledInSDK()) {
      flexCounterMgr->attachToPort(gport_);
    }
  }
  initCustomStats();

  // Notify platform port of initial state
  getPlatformPort()->linkStatusChanged(isUp(), isEnabledInSDK());
  getPlatformPort()->externalState(PortLedExternalState::NONE);

  if (isEnabledInSDK()) {
    enableLinkscan();
  }
}

bool BcmPort::supportsSpeed(cfg::PortSpeed speed) {
  // It would be nice if we could use the port_ability api here, but
  // that struct changes based on how many lanes are active. So does
  // bcm_port_speed_max.
  //
  // Instead, we store the speed set in the bcm config file. This will
  // not work correctly if we performed a warm boot and the config
  // file changed port speeds. However, this is not supported by
  // broadcom for warm boot so this approach should be alright.
  return speed <= getMaxSpeed();
}

bcm_pbmp_t BcmPort::getPbmp() {
  bcm_pbmp_t pbmp;
  BCM_PBMP_PORT_SET(pbmp, port_);
  return pbmp;
}

void BcmPort::disable(const std::shared_ptr<Port>& swPort) {
  if (!isEnabledInSDK()) {
    // Already disabled
    XLOG(DBG2) << "No need to disable port " << port_
               << " since it is already disabled";
    return;
  }

  XLOG(DBG1) << "Disabling port " << port_;

  auto pbmp = getPbmp();
  for (auto entry : swPort->getVlans()) {
    auto rv = bcm_vlan_port_remove(unit_, entry.first, pbmp);
    bcmCheckError(
        rv,
        "failed to remove disabled port ",
        swPort->getID(),
        " from VLAN ",
        entry.first);
  }

  disableStatCollection();

  // Disable sFlow sampling
  disableSflow();

  auto rv = bcm_port_enable_set(unit_, port_, false);
  bcmCheckError(rv, "failed to disable port ", swPort->getID());
}

void BcmPort::disableLinkscan() {
  int rv = bcm_linkscan_mode_set(unit_, port_, BCM_LINKSCAN_MODE_NONE);
  bcmCheckError(rv, "Failed to disable linkscan on port ", port_);
}

bool BcmPort::isEnabled() const {
  auto settings = getProgrammedSettings();
  if (settings) {
    return settings->isEnabled();
  }
  return isEnabledInSDK();
}

bool BcmPort::isEnabledInSDK() const {
  int enabled;
  auto rv = bcm_port_enable_get(unit_, port_, &enabled);
  bcmCheckError(rv, "Failed to determine if port is already disabled");
  return static_cast<bool>(enabled);
}

bool BcmPort::isUp() const {
  if (!isEnabled()) {
    return false;
  }
  int linkStatus;
  auto rv = bcm_port_link_status_get(hw_->getUnit(), port_, &linkStatus);
  bcmCheckError(rv, "could not find if the port ", port_, " is up or down...");
  return linkStatus == BCM_PORT_LINK_STATUS_UP;
}

bool BcmPort::isProgrammed() {
  return *programmedSettings_.rlock() != nullptr;
}

void BcmPort::enable(const std::shared_ptr<Port>& swPort) {
  if (isEnabledInSDK()) {
    // Port is already enabled, don't need to do anything
    XLOG(DBG2) << "No need to enable port " << port_
               << " since it is already enabled";
    return;
  }

  XLOG(DBG1) << "Enabling port " << port_;

  auto pbmp = getPbmp();
  bcm_pbmp_t emptyPortList;
  BCM_PBMP_CLEAR(emptyPortList);
  int rv;
  for (auto entry : swPort->getVlans()) {
    if (!entry.second.tagged) {
      rv = bcm_vlan_port_add(unit_, entry.first, pbmp, pbmp);
    } else {
      rv = bcm_vlan_port_add(unit_, entry.first, pbmp, emptyPortList);
    }
    bcmCheckError(
        rv,
        "failed to add enabled port ",
        swPort->getID(),
        " to VLAN ",
        entry.first);
  }

  // Drop packets to/from this port that are tagged with a VLAN that this
  // port isn't a member of.
  rv = bcm_port_vlan_member_set(
      unit_, port_, BCM_PORT_VLAN_MEMBER_INGRESS | BCM_PORT_VLAN_MEMBER_EGRESS);
  bcmCheckError(rv, "failed to set VLAN filtering on port ", swPort->getID());

  enableStatCollection(swPort);

  // Set the speed, ingress vlan, and sFlow rates before enabling
  program(swPort);

  // T66562358 We found out bcm_mirror_init() always disable IP on logical port
  // 136 and eventually discard any L3 packets to this port.
  // To avoid depending on Broadcom SDK to enable IP for us, we can always
  // enable both V4 and V6 L3 if we need to enable a port.
  enableL3(true, true);

  rv = bcm_port_enable_set(unit_, port_, true);
  bcmCheckError(rv, "failed to enable port ", swPort->getID());
}

void BcmPort::enableLinkscan() {
  int rv = bcm_linkscan_mode_set(unit_, port_, BCM_LINKSCAN_MODE_SW);
  bcmCheckError(rv, "Failed to enable linkscan on port ", port_);
}

void BcmPort::program(const shared_ptr<Port>& port) {
  // This function must have two properties:
  // 1) idempotency
  // 2) no port flaps if called twice with same settings on a running port

  XLOG(DBG1) << "Reprogramming BcmPort for port " << port->getID();
  setIngressVlan(port);
  if (platformPort_->shouldUsePortResourceAPIs()) {
    if (setPortResource(port)) {
      reinitPortFdrStats(port);
    }
  } else {
    setSpeed(port);
    // Update FEC settings if needed. Note this is not only
    // on speed change as the port's default speed (say on a
    // cold boot) maybe what is desired by the config. But we
    // may still need to enable FEC
    if (setFEC(port)) {
      // Note that setFEC() ignore the FEC change if port is up to avoid
      // flapping the port. So we need to detect the change here
      // instead of diffing the new and old config like
      // setupStatsIfNeeded() does below.
      reinitPortFdrStats(port);
    }
  }

  // setting sflow rates must come before setting sample destination.
  setSflowRates(port);

  // If no sample destination is provided, we configure it to be CPU, which is
  // the switch's default sample destination configuration
  cfg::SampleDestination dest = cfg::SampleDestination::CPU;
  if (port->getSampleDestination().has_value()) {
    dest = port->getSampleDestination().value();
  }

  /* update mirrors for port, mirror add/update must happen earlier than
   * updating mirrors for port */
  updateMirror(port->getIngressMirror(), MirrorDirection::INGRESS, dest);
  updateMirror(port->getEgressMirror(), MirrorDirection::EGRESS, dest);

  if (port->getQosPolicy()) {
    attachIngressQosPolicy(port->getQosPolicy().value());
  } else {
    detachIngressQosPolicy();
  }

  setPause(port);

  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    setPfc(port);
    if (!isMmuLossy()) {
      setCosqProfile(getDefaultProfileId());
    }
    ingressBufferManager_->programIngressBuffers(port);
  }

  // Update Tx Setting if needed.
  setTxSetting(port);
  setLoopbackMode(port->getLoopbackMode());

  setupStatsIfNeeded(port);
  setupPrbs(port);

  // Update Inter-packet frame if needed.
  phy::ProfileSideConfig profileConfig = port->getProfileConfig();
  if (auto ipgBits = profileConfig.interPacketGapBits()) {
    setInterPacketGapBits(*ipgBits);
  }

  if (!isProgrammed()) {
    // enable linkscan when port is programmed for the first time
    enableLinkscan();
  }
  // Cache the number of phy lanes this port uses
  auto lanes = port->getPinConfigs().size();
  numLanes_.exchange(lanes);

  {
    XLOG(DBG3) << "Saving port settings for " << port->getName();
    auto lockedSettings = programmedSettings_.wlock();
    *lockedSettings = port;
  }
}

void BcmPort::cacheFaultStatus(phy::LinkFaultStatus faultStatus) {
  auto faultStatusPtr = cachedFaultStatus.wlock();
  *faultStatusPtr = faultStatus;
}

void BcmPort::linkStatusChanged(const std::shared_ptr<Port>& port) {
  getPlatformPort()->linkStatusChanged(port->isUp(), port->isEnabled());
}

void BcmPort::setIngressVlan(const shared_ptr<Port>& swPort) {
  bcm_vlan_t currVlan;
  auto rv = bcm_port_untagged_vlan_get(unit_, port_, &currVlan);
  bcmCheckError(rv, "failed to get ingress VLAN for port ", swPort->getID());

  bcm_vlan_t bcmVlan = swPort->getIngressVlan();
  if (bcmVlan != currVlan) {
    rv = bcm_port_untagged_vlan_set(unit_, port_, bcmVlan);
    bcmCheckError(
        rv,
        "failed to set ingress VLAN for port ",
        swPort->getID(),
        " from ",
        currVlan,
        " to ",
        bcmVlan);
  }
}

bcm_port_if_t BcmPort::getDesiredInterfaceMode(
    cfg::PortSpeed speed,
    const std::shared_ptr<Port>& swPort) {
  const auto profileID = swPort->getProfileID();
  const auto& portProfileConfig = swPort->getProfileConfig();
  // All profiles should have medium info at this point
  if (!portProfileConfig.medium()) {
    throw FbossError(
        "Missing medium info in profile ",
        apache::thrift::util::enumNameSafe(profileID));
  }
  TransmitterTechnology transmitterTech = *portProfileConfig.medium();

  // If speed or transmitter type isn't in map
  try {
    auto result =
        getSpeedToTransmitterTechAndMode().at(speed).at(transmitterTech);
    XLOG(DBG1) << "Getting desired interface mode for port " << swPort->getID()
               << " (speed=" << apache::thrift::util::enumNameSafe(speed)
               << ", tech="
               << apache::thrift::util::enumNameSafe(transmitterTech)
               << "). RESULT=" << result;
    return result;
  } catch (const std::out_of_range& ex) {
    throw FbossError(
        "Unsupported speed (",
        apache::thrift::util::enumNameSafe(speed),
        ") or transmitter technology (",
        apache::thrift::util::enumNameSafe(transmitterTech),
        ") setting on port ",
        swPort->getID());
  }
}

cfg::PortSpeed BcmPort::getSpeed() const {
  int curSpeed{0};
  if (platformPort_->shouldUsePortResourceAPIs()) {
    curSpeed = getCurrentPortResource(unit_, gport_).speed;
  } else {
    auto rv = bcm_port_speed_get(unit_, port_, &curSpeed);
    bcmCheckError(rv, "Failed to get current speed for port ", port_);
  }
  return cfg::PortSpeed(curSpeed);
}

cfg::PortSpeed BcmPort::getDesiredPortSpeed(
    const std::shared_ptr<Port>& swPort) {
  if (swPort->getSpeed() == cfg::PortSpeed::DEFAULT) {
    int speed;
    auto ret = bcm_port_speed_max(unit_, port_, &speed);
    bcmCheckError(ret, "failed to get max speed for port", swPort->getID());
    return cfg::PortSpeed(speed);
  } else {
    return swPort->getSpeed();
  }
}

uint8_t BcmPort::determinePipe() const {
  // almost certainly open sourced since we updated bcm...
  bcm_info_t info;
  auto rv = bcm_info_get(unit_, &info);
  bcmCheckError(rv, "failed to get unit info");

#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20))
  if (info.num_pipes > BCM_PIPES_MAX) {
    // Tomahawk4 has 16 data pipes larger than old BCM_PIPES_MAX(8)
    bcm_pbmp_t pbmp;
    for (int i = 0; i < info.num_pipes; ++i) {
      rv = bcm_port_pipe_pbmp_get(unit_, i, &pbmp);
      bcmCheckError(rv, "failed to get port pbmp for pipe ", i);
      if (BCM_PBMP_MEMBER(pbmp, port_)) {
        return i;
      }
    }
    throw FbossError("Port ", port_, " not associated w/ any pipe");
  }
#endif

  bcm_port_config_t portConfig;
  bcm_port_config_t_init(&portConfig);
  rv = bcm_port_config_get(unit_, &portConfig);
  bcmCheckError(rv, "failed to get port configuration");

  for (int i = 0; i < info.num_pipes; ++i) {
    if (BCM_PBMP_MEMBER(portConfig.per_pipe[i], port_)) {
      return i;
    }
  }

  throw FbossError("Port ", port_, " not associated w/ any pipe");
}

void BcmPort::setInterfaceMode(const shared_ptr<Port>& swPort) {
  auto desiredPortSpeed = getDesiredPortSpeed(swPort);
  bcm_port_if_t desiredMode = getDesiredInterfaceMode(desiredPortSpeed, swPort);

  // Check whether we have the correct interface set
  bcm_port_if_t curMode = bcm_port_if_t(0);
  auto ret = bcm_port_interface_get(unit_, port_, &curMode);
  bcmCheckError(
      ret,
      "Failed to get current interface setting for port ",
      swPort->getID());

  // HACK: we cannot call speed_set w/out also
  // calling interface_mode_set, otherwise the
  // interface mode may change unexpectedly (details
  // on T32158588). We call set_speed when the port
  // is down, so also set mode here.
  //
  // TODO(aeckert): evaluate if we still need to set
  // speed on down ports.

  bool portUp = swPort->isPortUp();
  if (curMode != desiredMode || !portUp) {
    // Changes to the interface setting only seem to take effect on the next
    // call to bcm_port_speed_set()
    ret = bcm_port_interface_set(unit_, port_, desiredMode);
    bcmCheckError(
        ret, "failed to set interface type for port ", swPort->getID());
  }
}

void BcmPort::setSpeed(const shared_ptr<Port>& swPort) {
  int ret;
  cfg::PortSpeed desiredPortSpeed = getDesiredPortSpeed(swPort);
  int desiredSpeed = static_cast<int>(desiredPortSpeed);
  // Unnecessarily updating BCM port speed actually causes
  // the port to flap, even if this should be a noop, so check current
  // speed before making speed related changes. Doing so fixes
  // the interface flaps we were seeing during warm boots

  int curSpeed = static_cast<int>(getSpeed());

  // If the port is down or disabled its safe to update mode and speed to
  // desired values
  bool portUp = swPort->isPortUp();

  // Update to correct mode and speed settings if the port is down/disabled
  // or if the speed changed. Ideally we would like to always update to the
  // desired mode and speed. However these changes are disruptive, in that
  // they cause a port flap. So to avoid that, we don't update to desired
  // mode if the port is UP and running at the desired speed. Speed changes
  // though are applied to UP ports as well, since running at wrong (lower than
  // desired) speed is pretty dangerous, and can trigger non obvious outages.
  //
  // Another practical reason for not updating to the desired mode on ports that
  // are UP is that there is at least one bug whereby SDK thinks that the ports
  // are in a different mode than they actually are. We are tracking that
  // separately. Once that is resolved, we can do a audit to see that if all
  // ports are in desired mode settings, we can make mode changes a first
  // class citizen as well.

  XLOG(DBG1) << "setSpeed called on port " << port_ << ": portUp=" << portUp
             << ", curSpeed=" << curSpeed << ", desiredSpeed=" << desiredSpeed;
  if (!portUp || curSpeed != desiredSpeed) {
    setInterfaceMode(swPort);

    if (portUp) {
      // Changing the port speed causes traffic disruptions, but not doing
      // it would cause inconsistency.  Warn the user.
      XLOG(WARNING) << "Changing port speed on up port. This will "
                    << "disrupt traffic. Port: " << swPort->getName()
                    << " id: " << swPort->getID();
    }

    XLOG(DBG1) << "Finalizing BcmPort::setSpeed() by calling port_speed_set on "
               << "port " << swPort->getID() << " (" << swPort->getName()
               << ")";

    // Note that we call speed_set even if the speed is already set
    // properly and port is down. This is because speed_set
    // reinitializes the MAC layer of the port and allows us to pick
    // up changes in interface mode and finalize flex port
    // transitions. We ensure that the port is down for these
    // potentially unnecessary calls, as otherwise this will cause
    // port flaps on ports where link is up.
    ret = bcm_port_speed_set(unit_, port_, desiredSpeed);
    bcmCheckError(
        ret,
        "failed to set speed to ",
        desiredSpeed,
        " from ",
        curSpeed,
        ", on port ",
        swPort->getID());
  }
}

int BcmPort::sampleDestinationToBcmDestFlag(cfg::SampleDestination dest) {
  switch (dest) {
    case cfg::SampleDestination::CPU:
      return BCM_PORT_CONTROL_SAMPLE_DEST_CPU;
    case cfg::SampleDestination::MIRROR:
      return BCM_PORT_CONTROL_SAMPLE_DEST_MIRROR;
  }
  throw FbossError("Invalid sample destination", dest);
}

void BcmPort::configureSampleDestination(cfg::SampleDestination sampleDest) {
  sampleDest_ = sampleDest;

  if (!getHW()->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::SFLOW_SAMPLING)) {
    return;
  }

  auto rv = bcm_port_control_set(
      unit_,
      port_,
      bcmPortControlSampleIngressDest,
      sampleDestinationToBcmDestFlag(sampleDest_));
  bcmCheckError(
      rv,
      folly::sformat(
          "Failed to set sample destination for port {} : {}",
          port_,
          bcm_errmsg(rv)));
  return;
}

/**
 * Enable V4 and V6 L3
 */
void BcmPort::enableL3(bool enableV4, bool enableV6) {
  std::array<std::tuple<std::string, bcm_port_control_t, bool>, 2> l3Options = {
      std::make_tuple("bcmPortControlIP4", bcmPortControlIP4, enableV4),
      std::make_tuple("bcmPortControlIP6", bcmPortControlIP6, enableV6)};

  for (auto l3Option : l3Options) {
    int currVal{0};
    auto rv =
        bcm_port_control_get(unit_, port_, std::get<1>(l3Option), &currVal);
    bcmCheckError(
        rv,
        folly::sformat(
            "Failed to get {} for port {} : {}",
            std::get<0>(l3Option),
            port_,
            bcm_errmsg(rv)));
    if (currVal != ((std::get<2>(l3Option)) ? 1 : 0)) {
      XLOG(DBG2) << "Current " << std::get<0>(l3Option) << " for port " << port_
                 << " is " << (currVal ? "ENABLED" : "DISABLED")
                 << ", about to program to "
                 << (std::get<2>(l3Option) ? "ENABLED" : "DISABLED");
      rv = bcm_port_control_set(
          unit_,
          port_,
          std::get<1>(l3Option),
          static_cast<int>(std::get<2>(l3Option)));
      bcmCheckError(
          rv,
          folly::sformat(
              "Failed to set {} for port {} to {} : {}",
              std::get<0>(l3Option),
              port_,
              (std::get<2>(l3Option) ? "ENABLED" : "DISABLED"),
              bcm_errmsg(rv)));
    } else {
      XLOG(DBG5) << "No need to program port control L3. "
                 << "Current " << std::get<0>(l3Option) << " for port " << port_
                 << " is " << (currVal ? "ENABLED" : "DISABLED")
                 << ", which is the same to expected: "
                 << (std::get<2>(l3Option) ? "ENABLED" : "DISABLED");
    }
  }
}

void BcmPort::setupStatsIfNeeded(const std::shared_ptr<Port>& swPort) {
  auto lockedPortStatsPtr = portStats_.wlock();
  if (!lockedPortStatsPtr->has_value()) {
    return;
  }

  std::shared_ptr<Port> savedPort = getProgrammedSettings();

  if (!savedPort || swPort->getName() != savedPort->getName() ||
      hasPortQueueChanges(savedPort, swPort) ||
      hasPfcStatusChangedToEnabled(savedPort, swPort)) {
    reinitPortStatsLocked(lockedPortStatsPtr, swPort);
  }
  if (savedPort && hasPfcStatusChangedToDisabled(savedPort, swPort)) {
    // Remove stats in case PFC is disabled for previously enabled priorities
    auto pfcPriorities = savedPort->getPfcPriorities();
    removePortPfcStatsLocked(lockedPortStatsPtr, swPort, pfcPriorities);
  }

  // Set bcmPortControlStatOversize to max frame size so that we don't trigger
  // OVR counters increment due to jumbo frame
  auto rv = bcm_port_control_set(
      unit_, port_, bcmPortControlStatOversize, swPort->getMaxFrameSize());
  bcmCheckError(
      rv,
      "Failed to set bcmPortControlStatOversize for port ",
      port_,
      " to size=",
      swPort->getMaxFrameSize());
}

void BcmPort::setupPrbs(const std::shared_ptr<Port>& swPort) {
  auto prbsState = swPort->getAsicPrbs();
  if (*prbsState.enabled()) {
    auto prbsMap = asicPrbsPolynomialMap();
    auto asicPrbsPolynominalIter = prbsMap.left.find(*prbsState.polynominal());
    if (asicPrbsPolynominalIter == prbsMap.left.end()) {
      throw FbossError(
          "Polynominal value not supported: ", *prbsState.polynominal());
    } else {
      auto rv = bcm_port_phy_control_set(
          unit_,
          port_,
          BCM_PORT_PHY_CONTROL_PRBS_POLYNOMIAL,
          asicPrbsPolynominalIter->second);

      bcmCheckError(
          rv, "failed to set prbs polynomial for port ", swPort->getID());
    }
  }

  std::string enableStr = *prbsState.enabled() ? "enable" : "disable";

  auto setPrbsIfNeeded = [&](bcm_port_phy_control_t type) {
    std::string typeStr = (type == BCM_PORT_PHY_CONTROL_PRBS_TX_ENABLE)
        ? "BCM_PORT_PHY_CONTROL_PRBS_TX_ENABLE"
        : "BCM_PORT_PHY_CONTROL_PRBS_RX_ENABLE";
    uint32 currVal{0};
    auto rv = bcm_port_phy_control_get(unit_, port_, type, &currVal);
    if (rv != BCM_E_NOT_FOUND) {
      bcmCheckError(
          rv,
          folly::sformat(
              "Failed to get {} for port {} : {}",
              typeStr,
              port_,
              bcm_errmsg(rv)));
    }

    if (rv == BCM_E_NOT_FOUND || currVal != (*prbsState.enabled() ? 1 : 0)) {
      rv = bcm_port_phy_control_set(
          unit_, port_, type, ((*prbsState.enabled()) ? 1 : 0));

      bcmCheckError(
          rv,
          folly::sformat(
              "Setting {} {} failed for port {}", typeStr, enableStr, port_));
    } else {
      XLOG(DBG2) << typeStr << " is already " << enableStr << " for port "
                 << port_;
    }
  };

  setPrbsIfNeeded(BCM_PORT_PHY_CONTROL_PRBS_TX_ENABLE);
  setPrbsIfNeeded(BCM_PORT_PHY_CONTROL_PRBS_RX_ENABLE);
}

std::vector<prbs::PrbsPolynomial> BcmPort::getPortPrbsPolynomials() {
  std::vector<prbs::PrbsPolynomial> capabilities;
  for (auto it : asicPrbsPolynomialMap().left) {
    capabilities.push_back(static_cast<prbs::PrbsPolynomial>(it.first));
  }
  return capabilities;
}

PortID BcmPort::getPortID() const {
  return platformPort_->getPortID();
}

std::string BcmPort::getPortName() const {
  // TODO: replace with pulling name from platform port
  auto prevSettings = programmedSettings_.rlock();
  if (!*prevSettings) {
    return folly::to<std::string>("port", getPortID());
  }
  return (*prevSettings)->getName();
}

LaneSpeeds BcmPort::supportedLaneSpeeds() const {
  return platformPort_->supportedLaneSpeeds();
}

std::shared_ptr<Port> BcmPort::getSwitchStatePort(
    const std::shared_ptr<SwitchState>& state) const {
  return state->getPort(getPortID());
}

std::shared_ptr<Port> BcmPort::getSwitchStatePortIf(
    const std::shared_ptr<SwitchState>& state) const {
  return state->getPorts()->getNodeIf(getPortID());
}

void BcmPort::registerInPortGroup(BcmPortGroup* portGroup) {
  portGroup_ = portGroup;
  XLOG(DBG2) << "Port " << getPortID() << " registered in PortGroup with "
             << "controlling port "
             << portGroup->controllingPort()->getPortID();
}

std::string BcmPort::statName(
    folly::StringPiece statName,
    folly::StringPiece portName) const {
  return folly::to<string>(portName, ".", statName);
}

phy::PhyInfo BcmPort::updateIPhyInfo() {
  phy::PhyInfo info;
  phy::PhyState state;
  phy::PhyStats stats;

  info.name() = getPortName();
  state.name() = *info.name();

  // Global phy parameters
  phy::DataPlanePhyChip phyChip;
  phyChip.type() = phy::DataPlanePhyChipType::IPHY;
  info.phyChip() = phyChip;
  state.phyChip() = phyChip;
  info.speed() = getSpeed();
  state.speed() = *info.speed();
  info.linkState() = isUp();
  state.linkState() = *info.linkState();

  // PCS parameters
  phy::PcsInfo pcs;
  phy::PcsStats pcsStats;
  // FEC parameters
  if (auto portStats = getPortStats()) {
    auto fecMode = getFECMode();
    if (fecMode == phy::FecMode::CL91 || fecMode == phy::FecMode::RS528 ||
        fecMode == phy::FecMode::RS544 || fecMode == phy::FecMode::RS544_2N) {
      phy::RsFecInfo rsFec;
      rsFec.correctedCodewords() = *((*portStats).fecCorrectableErrors());
      rsFec.uncorrectedCodewords() = *((*portStats).fecUncorrectableErrors());

      phy::RsFecInfo lastRsFec;
      if (auto lastPcs = lastPhyInfo_.line()->pcs()) {
        if (auto lastFec = lastPcs->rsFec()) {
          lastRsFec = *lastFec;
        }
      }
      std::optional<uint64_t> correctedBitsFromHw;
#if defined(BCM_SDK_VERSION_GTE_6_5_26)
      if (hw_->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::FEC_CORRECTED_BITS)) {
        int64_t correctedBits;
        fec_stat_accumulate(
            unit_,
            port_,
            BCM_PORT_PHY_CONTROL_RS_FEC_BIT_ERROR_COUNT,
            &correctedBits);
        correctedBitsFromHw = *lastRsFec.correctedBits() + correctedBits;
      }
#endif
      auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
      utility::updateCorrectedBitsAndPreFECBer(
          rsFec, /* current RsFecInfo to update */
          lastRsFec, /* previous RsFecInfo */
          correctedBitsFromHw, /* counter if available from hardware */
          now.count() - *lastPhyInfo_.timeCollected(), /* timeDeltaInSeconds */
          fecMode, /* operational FecMode */
          *info.speed() /* operational Speed */);
      pcs.rsFec() = rsFec;
      pcsStats.rsFec() = rsFec;
    }
  }

  // PMD Parameters
  phy::PmdInfo pmd;
  phy::PmdState pmdState;
  phy::PmdStats pmdStats;
  int totalPmdLanes = numLanes_;
  phy::PmdInfo lastPmd = *lastPhyInfo_.line()->pmd();
  phy::PmdState lastPmdState;
  if (auto lastState = lastPhyInfo_.state()) {
    lastPmdState = *lastState->line()->pmd();
  }
  phy::PmdStats lastPmdStats;
  if (auto lastStats = lastPhyInfo_.stats()) {
    lastPmdStats = *lastStats->line()->pmd();
  }
  bool readRxFreq = hw_->getPlatform()->getAsic()->isSupported(
      HwAsic::Feature::RX_FREQUENCY_PPM);
  // On TH4, BCM_PORT_PHY_CONTROL_RX_PPM is only supported from 6.5.28
  if (readRxFreq &&
      hw_->getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
#if defined(BCM_SDK_VERSION_GTE_6_5_28)
    readRxFreq = true;
#else
    readRxFreq = false;
#endif
  }
  for (int lane = 0; lane < totalPmdLanes; lane++) {
    phy::LaneInfo laneInfo;
    phy::LaneState laneState;
    laneInfo.lane_ref() = lane;
    laneState.lane_ref() = lane;
    if (readRxFreq) {
      uint32_t value;
      auto rv = bcm_port_phy_control_get(
          unit_, port_, BCM_PORT_PHY_CONTROL_RX_PPM, &value);
      bcmCheckError(rv, "failed to get rx frequency ppm");
      laneInfo.rxFrequencyPPM() = value;
      laneState.rxFrequencyPPM() = value;
    }
    pmd.lanes_ref()[lane] = laneInfo;
    pmdState.lanes_ref()[lane] = laneState;
  }
#if defined(BCM_SDK_VERSION_GTE_6_5_24)
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PMD_RX_LOCK_STATUS)) {
    bcm_port_pmd_rx_lock_status_t lock_status;
    auto rv = bcm_port_pmd_rx_lock_status_get(unit_, port_, &lock_status);
    if (!BCM_FAILURE(rv)) {
      for (int lane = 0; lane < totalPmdLanes; lane++) {
        pmd.lanes_ref()[lane].lane_ref() = lane;
        pmdState.lanes_ref()[lane].lane_ref() = lane;
        pmdStats.lanes_ref()[lane].lane_ref() = lane;
        pmd.lanes_ref()[lane].cdrLockLive_ref() =
            lock_status.rx_lock_bmp & (1 << lane);
        pmdState.lanes_ref()[lane].cdrLockLive_ref() =
            lock_status.rx_lock_bmp & (1 << lane);
        bool changed = lock_status.rx_lock_change_bmp & (1 << lane);
        pmd.lanes_ref()[lane].cdrLockChanged_ref() = changed;
        pmdState.lanes_ref()[lane].cdrLockChanged_ref() = changed;
        utility::updateCdrLockChangedCount(
            changed, lane, pmd.lanes_ref()[lane], lastPmd);
        utility::updateCdrLockChangedCount(
            changed, lane, pmdStats.lanes_ref()[lane], lastPmdStats);
      }
    } else {
      XLOG(ERR) << "Failed to read rx_lock_status for port " << port_ << " :"
                << bcm_errmsg(rv);
    }
  }
#endif
#if defined(BCM_SDK_VERSION_GTE_6_5_26)
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PMD_RX_SIGNAL_DETECT)) {
    bcm_port_phy_signal_detect_status_t sd_status;
    auto rv = bcm_port_phy_signal_detect_status_get(unit_, port_, &sd_status);
    if (!BCM_FAILURE(rv)) {
      for (int lane = 0; lane < totalPmdLanes; lane++) {
        pmd.lanes_ref()[lane].lane_ref() = lane;
        pmdState.lanes_ref()[lane].lane_ref() = lane;
        pmdStats.lanes_ref()[lane].lane_ref() = lane;
        pmd.lanes_ref()[lane].signalDetectLive_ref() =
            sd_status.signal_detect_bmp & (1 << lane);
        pmdState.lanes_ref()[lane].signalDetectLive_ref() =
            sd_status.signal_detect_bmp & (1 << lane);
        bool changed = sd_status.signal_detect_change_bmp & (1 << lane);
        pmd.lanes_ref()[lane].signalDetectChanged_ref() = changed;
        pmdState.lanes_ref()[lane].signalDetectChanged_ref() = changed;
        utility::updateSignalDetectChangedCount(
            changed, lane, pmd.lanes_ref()[lane], lastPmd);
        utility::updateSignalDetectChangedCount(
            changed, lane, pmdStats.lanes_ref()[lane], lastPmdStats);
      }
    } else {
      XLOG(ERR) << "Failed to read rx_signal_detect_status for port " << port_
                << " :" << bcm_errmsg(rv);
    }
  }
#endif

  // Line side parameters
  phy::PhySideInfo lineSideInfo;
  lineSideInfo.side() = phy::Side::LINE;
  lineSideInfo.pcs() = pcs;
  lineSideInfo.pmd() = pmd;

  phy::PhySideState lineSideState;
  lineSideState.side() = phy::Side::LINE;
  lineSideState.pmd() = pmdState;

  phy::PhySideStats lineSideStats;
  lineSideStats.side() = phy::Side::LINE;
  lineSideStats.pcs() = pcsStats;
  lineSideStats.pmd() = pmdStats;

  // Local/Remote Fault Status
  auto faultStatusPtr = cachedFaultStatus.wlock();
  if (*faultStatusPtr) {
    phy::RsInfo rsInfo;
    rsInfo.faultStatus() = **faultStatusPtr;
    lineSideInfo.rs() = rsInfo;
    lineSideState.rs() = rsInfo;
    // Reset the cached status back to nullopt. We want to only update
    // the fault status in PhyInfo when port state update happens
    *faultStatusPtr = std::nullopt;
  }

  info.line() = lineSideInfo;
  state.line() = lineSideState;
  stats.line() = lineSideStats;

  // PhyInfo update timestamp
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  info.timeCollected() = now.count();
  state.timeCollected() = now.count();
  stats.timeCollected() = now.count();

  info.state() = state;
  info.stats() = stats;
  lastPhyInfo_ = info;
  return info;
}

void BcmPort::updateStats() {
  auto lockedPortStatsPtr = portStats_.wlock();
  if (!lockedPortStatsPtr->has_value()) {
    return;
  }

  // TODO: It would be nicer to use a monotonic clock, but unfortunately
  // the ServiceData code currently expects everyone to use system time.
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  HwPortStats curPortStats, lastPortStats;
  lastPortStats = curPortStats = (*lockedPortStatsPtr)->portStats();

  // All stats start with a unitialized (-1) value. If there are no in discards
  // we will just report that as the monotonic counter. Instead set it to
  // 0 if uninintialized
  *curPortStats.inDiscards_() = *curPortStats.inDiscards_() ==
          hardware_stats_constants::STAT_UNINITIALIZED()
      ? 0
      : *curPortStats.inDiscards_();
  curPortStats.timestamp_() = now.count();

  updateStat(now, kInBytes(), snmpIfHCInOctets, &(*curPortStats.inBytes_()));
  updateStat(
      now,
      kInUnicastPkts(),
      snmpIfHCInUcastPkts,
      &(*curPortStats.inUnicastPkts_()));
  updateStat(
      now,
      kInMulticastPkts(),
      snmpIfHCInMulticastPkts,
      &(*curPortStats.inMulticastPkts_()));
  updateStat(
      now,
      kInBroadcastPkts(),
      snmpIfHCInBroadcastPkts,
      &(*curPortStats.inBroadcastPkts_()));
  updateStat(now, kInErrors(), snmpIfInErrors, &(*curPortStats.inErrors_()));
  updateStat(
      now,
      kInIpv4HdrErrors(),
      snmpIpInHdrErrors,
      &(*curPortStats.inIpv4HdrErrors_()));
  updateStat(
      now,
      kInIpv6HdrErrors(),
      snmpIpv6IfStatsInHdrErrors,
      &(*curPortStats.inIpv6HdrErrors_()));
  updateStat(
      now, kInPause(), snmpDot3InPauseFrames, &(*curPortStats.inPause_()));
  // Egress Stats
  updateStat(now, kOutBytes(), snmpIfHCOutOctets, &(*curPortStats.outBytes_()));
  updateStat(
      now,
      kOutUnicastPkts(),
      snmpIfHCOutUcastPkts,
      &(*curPortStats.outUnicastPkts_()));
  updateStat(
      now,
      kOutMulticastPkts(),
      snmpIfHCOutMulticastPkts,
      &(*curPortStats.outMulticastPkts_()));
  updateStat(
      now,
      kOutBroadcastPkts(),
      snmpIfHCOutBroadcastPckts,
      &(*curPortStats.outBroadcastPkts_()));
  updateStat(
      now, kOutDiscards(), snmpIfOutDiscards, &(*curPortStats.outDiscards_()));
  updateStat(now, kOutErrors(), snmpIfOutErrors, &(*curPortStats.outErrors_()));
  updateStat(
      now, kOutPause(), snmpDot3OutPauseFrames, &(*curPortStats.outPause_()));

  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::ECN)) {
    // ECN stats not supported by TD2
    updateStat(
        now,
        kOutEcnCounter(),
        snmpBcmTxEcnErrors,
        &(*curPortStats.outEcnCounter_()));
  }
  updateStat(
      now,
      kInDstNullDiscards(),
      snmpBcmCustomReceive3,
      &(*curPortStats.inDstNullDiscards_()));

  auto settings = getProgrammedSettings();
  uint64_t inCongestionDiscards = 0;
  if (isMmuLossless()) {
    // although this stat can be derived in the lossy world as well, but use
    // case is only for mmu lossless
    updateInCongestionDiscardStats(now, &inCongestionDiscards);
    *curPortStats.inCongestionDiscards_() = inCongestionDiscards;
  }

  // InDiscards will be read along with PFC if PFC is enabled
  if (settings && settings->getPfc().has_value()) {
    auto pfcPriorities = settings->getPfcPriorities();
    updatePortPfcStats(now, curPortStats, pfcPriorities);
  } else {
    updateStat(
        now,
        kInDiscardsRaw(),
        snmpIfInDiscards,
        &(*curPortStats.inDiscardsRaw_()));
  }
  updateFecStats(now, curPortStats);
  updateFdrStats(now);
  updateWredStats(now, &(*curPortStats.wredDroppedPackets_()));
  queueManager_->updateQueueStats(now, &curPortStats);

  std::vector<utility::CounterPrevAndCur> toSubtractFromInDiscardsRaw = {
      {*lastPortStats.inDstNullDiscards_(),
       *curPortStats.inDstNullDiscards_()}};
  if (isMmuLossy() &&
      hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::IN_PAUSE_INCREMENTS_DISCARDS)) {
    // If MMU setup as lossy, all incoming pause frames will be
    // discarded and will count towards in discards. This makes in discards
    // counter somewhat useless. So instead calculate "in_non_pause_discards",
    // by subtracting the pause frames received from in_discards.
    // TODO: Test if this is true when rx pause is enabled
    toSubtractFromInDiscardsRaw.emplace_back(
        *lastPortStats.inPause_(), *curPortStats.inPause_());
  }
  // as with pause, remove incoming PFC frames from the discards
  // as well
  if (curPortStats.inPfcCtrl_() !=
      hardware_stats_constants::STAT_UNINITIALIZED()) {
    toSubtractFromInDiscardsRaw.emplace_back(
        *lastPortStats.inPfcCtrl_(), *curPortStats.inPfcCtrl_());
  }

  *curPortStats.inDiscards_() += utility::subtractIncrements(
      {*lastPortStats.inDiscardsRaw_(), *curPortStats.inDiscardsRaw_()},
      toSubtractFromInDiscardsRaw);

  auto inDiscards = getPortCounterIf(kInDiscards());
  inDiscards->updateValue(now, *curPortStats.inDiscards_());

  *lockedPortStatsPtr = BcmPortStats(curPortStats, now);

  // Update the queue length stat
  uint32_t qlength;
  auto ret = bcm_port_queued_count_get(unit_, port_, &qlength);
  if (BCM_FAILURE(ret)) {
    XLOG(ERR) << "Failed to get queue length for port " << port_ << " :"
              << bcm_errmsg(ret);
  } else {
    outQueueLen_.addValue(now.count(), qlength);
    // TODO: outQueueLen_ only exports the average queue length over the last
    // 60 seconds, 10 minutes, etc.
    // We should also export the current value.  We could use a simple counter
    // or a dynamic counter for this.
  }

  // Update the packet length histograms
  updatePktLenHist(now, &inPktLengths_, kInPktLengthStats);
  updatePktLenHist(now, &outPktLengths_, kOutPktLengthStats);

  // Update any platform specific port counters
  getPlatformPort()->updateStats();
};

void BcmPort::updateFecStats(
    std::chrono::seconds now,
    HwPortStats& curPortStats) {
  // Only collect FEC stats on TH3 (will fail if we try to read the fec type
  // via port resource on TH)
  if (!platformPort_->shouldUsePortResourceAPIs()) {
    return;
  }
  bcm_port_phy_control_t corrected_ctrl, uncorrected_ctrl;
  bcm_port_phy_fec_t fec;
  // get fec from programmed cache to improve performance if possible
  auto settings = getProgrammedSettings();
  if (settings) {
    // With the profileConfig is stored in Port, we can just read from the
    // programmed settings.
    fec = utility::phyFecModeToBcmPortPhyFec(
        platformPort_->shouldDisableFEC()
            ? phy::FecMode::NONE
            : *settings->getProfileConfig().fec());
  } else {
    fec = getCurrentPortResource(unit_, gport_).fec_type;
  }
  // RS-FEC errors are in the CODEWORD_COUNT registers, while BaseR fec
  // errors are in the BLOCK_COUNT registers.
  if ((fec == bcmPortPhyFecRs544) || (fec == bcmPortPhyFecRs272) ||
      (fec == bcmPortPhyFecRs544_2xN) || (fec == bcmPortPhyFecRs272_2xN) ||
      (fec == bcmPortPhyFecRsFec)) {
    corrected_ctrl = BCM_PORT_PHY_CONTROL_FEC_CORRECTED_CODEWORD_COUNT;
    uncorrected_ctrl = BCM_PORT_PHY_CONTROL_FEC_UNCORRECTED_CODEWORD_COUNT;
  } else { /* Assume other FEC is BaseR */
    corrected_ctrl = BCM_PORT_PHY_CONTROL_FEC_CORRECTED_BLOCK_COUNT;
    uncorrected_ctrl = BCM_PORT_PHY_CONTROL_FEC_UNCORRECTED_BLOCK_COUNT;
  }

  fec_stat_accumulate(
      unit_, port_, corrected_ctrl, &(*curPortStats.fecCorrectableErrors()));
  fec_stat_accumulate(
      unit_,
      port_,
      uncorrected_ctrl,
      &(*curPortStats.fecUncorrectableErrors()));

  getPortCounterIf(kFecCorrectable())
      ->updateValue(now, *curPortStats.fecCorrectableErrors());
  getPortCounterIf(kFecUncorrectable())
      ->updateValue(now, *curPortStats.fecUncorrectableErrors());
}

void BcmPort::updateFdrStats(__attribute__((unused)) std::chrono::seconds now) {
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::FEC_DIAG_COUNTERS)) {
    return;
  }

  bcm_port_fdr_stats_t fdr_stats;
  if (BCM_FAILURE(bcm_port_fdr_stats_get(unit_, port_, &fdr_stats))) {
    return;
  }

  // Currently, we bounce between the 2 pages if there's more than 1 page,
  // just like the bin_group=both setting in the "phydiag <x> fdrstat start"
  // command in the Broadcom sdklt CLI does. Their source code is at
  // platform009/a0eedbb/src/libs/sdklt/bcma/bcmpc/diag/bcma_bcmpc_diag_fdrstat.c
  // in 6.5.22+ of the Broadcom SDK. (6.5.21 SDK does have the API but no
  // CLI.)
  //
  // When we collect counters from page 0, the errors in page 1 will not be
  // counted. So we'll miss half of the errors. But these errors should be
  // stable for long enough for us to catch them. Getting the full
  // distribution of errors is more helpful than an exact count. So we'll
  // just multiply the counts by 2x to approximate the correct values.
  // Similar to how interlaced videos put out alternating lines for 2
  // frames.
  //
  // Just bear in mind that for FEC modes with more than 1 page, the numbers
  // are extrapolated and not 100% accurate. But the relative distribution
  // between the different error bins should be correct.
  auto pages = getFecMaxErrors() / kCodewordErrorsPageSize;
  __attribute__((unused)) int codewordErrorBase =
      codewordErrorsPage_ * kCodewordErrorsPageSize;
  __attribute__((unused)) int increments[kCodewordErrorsPageSize];
  // Multiply by number of pages to pretend that this number showed up on
  // every page. Increment counter since this API is clear on read.
  increments[0] = fdr_stats.cw_s0_errs * pages;
  increments[1] = fdr_stats.cw_s1_errs * pages;
  increments[2] = fdr_stats.cw_s2_errs * pages;
  increments[3] = fdr_stats.cw_s3_errs * pages;
  increments[4] = fdr_stats.cw_s4_errs * pages;
  increments[5] = fdr_stats.cw_s5_errs * pages;
  increments[6] = fdr_stats.cw_s6_errs * pages;
  increments[7] = fdr_stats.cw_s7_errs * pages;

  for (int i = 0; i < kCodewordErrorsPageSize; ++i) {
    fdrStats_[i].incrementValue(now, increments[i]);
  }

  if (pages > 1) {
    codewordErrorsPage_ = (codewordErrorsPage_ + 1) % pages;
    fdrStatConfigure(unit_, port_, true, codewordErrorsPage_);
  }
#endif
}

void BcmPort::BcmPortStats::setQueueWaterMarks(
    std::map<int16_t, int64_t> queueId2WatermarkBytes) {
  *portStats_.queueWatermarkBytes_() = std::move(queueId2WatermarkBytes);
}

void BcmPort::setQueueWaterMarks(
    std::map<int16_t, int64_t> queueId2WatermarkBytes) {
  auto lockedPortStatsPtr = portStats_.wlock();
  if (!lockedPortStatsPtr->has_value()) {
    return;
  }
  (*lockedPortStatsPtr)->setQueueWaterMarks(std::move(queueId2WatermarkBytes));
}

std::string BcmPort::getPfcPriorityStatsKey(
    folly::StringPiece statKey,
    PfcPriority priority) {
  return folly::to<std::string>(statKey, ".priority", priority);
}

void BcmPort::updatePortPfcStats(
    std::chrono::seconds now,
    HwPortStats& portStats,
    const std::vector<PfcPriority>& pfcPriorities) {
  // Update per priority statistics for the priorities
  // which are enabled for PFC!
  for (auto pfcPri : pfcPriorities) {
    updateStat(
        now,
        getPfcPriorityStatsKey(kInPfc(), pfcPri),
        kInPfcStats.at(pfcPri),
        &(portStats.inPfc_()[pfcPri]));
    updateStat(
        now,
        getPfcPriorityStatsKey(kInPfcXon(), pfcPri),
        kInPfcXonStats.at(pfcPri),
        &(portStats.inPfcXon_()[pfcPri]));
    updateStat(
        now,
        getPfcPriorityStatsKey(kOutPfc(), pfcPri),
        kOutPfcStats.at(pfcPri),
        &(portStats.outPfc_()[pfcPri]));
  }

  // out pfc collection
  updateStat(
      now, kOutPfc(), snmpBcmTxPFCControlFrame, &(*portStats.outPfcCtrl_()));

  /*
   * PFC count will be subtracted from the discards to compute the actual
   * discards Read discards first followed by PFC count. This will ensure that
   * PFC count >= discards and so discards wont show PFC packets in them
   * (discards cannot go negative and so it will be 0).
   */
  bcm_stat_val_t statTypes[2] = {snmpIfInDiscards, snmpBcmRxPFCControlFrame};
  /*
   * Update per port PFC statistics. Use the Multi Get API to retrieve both
   * PFC count and Discard count. This will ensure that in an environment where
   * there are high PFC packets, the PFC packets sent in the gap between the
   * collection of the 2 stats does not show up as discards (T130263331)
   */
  updateMultiStat(
      now,
      {kInDiscardsRaw(), kInPfc()},
      statTypes,
      {&(*portStats.inDiscardsRaw_()), &(*portStats.inPfcCtrl_())},
      2);
}

void BcmPort::updateMultiStat(
    std::chrono::seconds now,
    std::vector<folly::StringPiece> statKeys,
    bcm_stat_val_t* types,
    std::vector<int64_t*> portStatVals,
    uint8_t statsCount) {
  uint64_t values[statsCount];
  auto ret = bcm_stat_sync_multi_get(unit_, port_, statsCount, types, values);
  if (BCM_FAILURE(ret)) {
    XLOG(ERR) << "Failed to get multi stats for port " << port_ << " :"
              << bcm_errmsg(ret);
    return;
  }
  for (int i = 0; i < statsCount; ++i) {
    auto statKey = statKeys.at(i);
    auto type = types[i];
    auto statVal = portStatVals.at(i);
    auto stat = getPortCounterIf(statKey);
    auto value = values[i];

    if (stat == nullptr) {
      for (auto iter = portCounters_.begin(); iter != portCounters_.end();
           iter++) {
        XLOG(ERR) << "key: " << iter->first
                  << " , counter name: " << iter->second.getName();
      }
      XLOG(FATAL) << "failed to find port counter for key " << statKey
                  << " and type " << type << ", existing counters:";
    }

    stat->updateValue(now, value);
    *statVal = value;
  }
}

void BcmPort::updateStat(
    std::chrono::seconds now,
    folly::StringPiece statKey,
    bcm_stat_val_t type,
    int64_t* statVal) {
  auto stat = getPortCounterIf(statKey);
  if (stat == nullptr) {
    for (auto iter = portCounters_.begin(); iter != portCounters_.end();
         iter++) {
      XLOG(ERR) << "key: " << iter->first
                << " , counter name: " << iter->second.getName();
    }
    XLOG(FATAL) << "failed to find port counter for key " << statKey
                << " and type " << type << ", existing counters:";
  }
  // Use the non-sync API to just get the values accumulated in software.
  // The Broadom SDK's counter thread syncs the HW counters to software every
  // 500000us (defined in config.bcm).
  uint64_t value;
  auto ret = bcm_stat_get(unit_, port_, type, &value);
  if (BCM_FAILURE(ret)) {
    XLOG(ERR) << "Failed to get stat " << type << " for port " << port_ << " :"
              << bcm_errmsg(ret);
    return;
  }
  stat->updateValue(now, value);
  *statVal = value;
}

void BcmPort::updateInCongestionDiscardStats(
    std::chrono::seconds now,
    uint64_t* portStatVal) {
  *portStatVal = 0;
  auto rv = bcm_cosq_stat_get(
      hw_->getUnit(),
      getBcmGport(),
      -1,
      bcmCosqStatSourcePortDroppedPackets,
      portStatVal);
  if (BCM_FAILURE(rv)) {
    XLOG(ERR) << "Failed to get ingress congestion discard stat "
              << " for port " << port_ << " :" << bcm_errmsg(rv);
  }
  auto stat = getPortCounterIf(kInCongestionDiscards());
  stat->updateValue(now, *portStatVal);
}

void BcmPort::updateWredStats(std::chrono::seconds now, int64_t* portStatVal) {
  auto getWredDroppedPackets = [this](auto statId) {
    uint64_t count{0};
    auto rv =
        bcm_cosq_stat_get(hw_->getUnit(), getBcmGport(), -1, statId, &count);
    if (BCM_FAILURE(rv)) {
      XLOG(ERR) << "Failed to get stat " << statId << " for port " << port_
                << " :" << bcm_errmsg(rv);
      return 0UL;
    }
    return count;
  };
  *portStatVal = getWredDroppedPackets(bcmCosqStatGreenDiscardDroppedPackets) +
      getWredDroppedPackets(bcmCosqStatYellowDiscardDroppedPackets) +
      getWredDroppedPackets(bcmCosqStatRedDiscardDroppedPackets);
  auto stat = getPortCounterIf(kWredDroppedPackets());
  stat->updateValue(now, *portStatVal);
}

bool BcmPort::isMmuLossy() const {
  return hw_->getMmuState() == BcmMmuState::MMU_LOSSY;
}

bool BcmPort::isMmuLossless() const {
  const auto mmuState = hw_->getMmuState();
  if ((mmuState == BcmMmuState::MMU_LOSSLESS) ||
      (mmuState == BcmMmuState::MMU_LOSSY_AND_LOSSLESS)) {
    return true;
  }
  return false;
}

void BcmPort::updatePktLenHist(
    std::chrono::seconds now,
    fb303::ExportedHistogramMapImpl::LockableHistogram* hist,
    const std::vector<bcm_stat_val_t>& stats) {
  // Get the counter values
  uint64_t counters[10];
  // bcm_stat_multi_get() unfortunately doesn't correctly const qualify
  // it's stats arguments right now.
  bcm_stat_val_t* statsArg = const_cast<bcm_stat_val_t*>(&stats.front());
  auto ret = bcm_stat_multi_get(unit_, port_, stats.size(), statsArg, counters);
  if (BCM_FAILURE(ret)) {
    XLOG(ERR) << "Failed to get packet length stats for port " << port_ << " :"
              << bcm_errmsg(ret);
    return;
  }

  // Update the histogram
  auto guard = hist->makeLockGuard();
  for (int idx = 0; idx < stats.size(); ++idx) {
    hist->addValueLocked(guard, now.count(), idx, counters[idx]);
  }
}

BcmPort::BcmPortStats::BcmPortStats(int numUnicastQueues) : BcmPortStats() {
  auto queueInitStats = folly::copy(*portStats_.queueOutDiscardBytes_());
  for (auto cosq = 0; cosq < numUnicastQueues; ++cosq) {
    queueInitStats.emplace(cosq, 0);
  }
  portStats_.queueOutDiscardBytes_() = queueInitStats;
  portStats_.queueOutBytes_() = queueInitStats;
  portStats_.queueOutPackets_() = queueInitStats;
  portStats_.queueWatermarkBytes_() = queueInitStats;
  portStats_.queueWredDroppedPackets_() = queueInitStats;
}

BcmPort::BcmPortStats::BcmPortStats(
    HwPortStats portStats,
    std::chrono::seconds timeRetrieved)
    : BcmPortStats() {
  portStats_ = portStats;
  timeRetrieved_ = timeRetrieved;
}

HwPortStats BcmPort::BcmPortStats::portStats() const {
  return portStats_;
}

std::chrono::seconds BcmPort::BcmPortStats::timeRetrieved() const {
  return timeRetrieved_;
}

std::optional<HwPortStats> BcmPort::getPortStats() const {
  auto lockedPortStatsPtr = portStats_.rlock();
  // we currently only target on enabled port
  if (!lockedPortStatsPtr->has_value()) {
    return std::nullopt;
  }
  return (*lockedPortStatsPtr)->portStats();
}

std::chrono::seconds BcmPort::getTimeRetrieved() const {
  auto lockedPortStatsPtr = portStats_.rlock();
  if (!lockedPortStatsPtr->has_value()) {
    return std::chrono::seconds(0);
  }
  return (*lockedPortStatsPtr)->timeRetrieved();
}

void BcmPort::applyMirrorAction(
    MirrorAction action,
    MirrorDirection direction,
    cfg::SampleDestination destination) {
  auto mirrorName =
      direction == MirrorDirection::INGRESS ? ingressMirror_ : egressMirror_;
  if (!mirrorName) {
    return;
  }
  auto* bcmMirror = hw_->getBcmMirrorTable()->getNodeIf(mirrorName.value());
  CHECK(bcmMirror != nullptr);
  bcmMirror->applyPortMirrorAction(getPortID(), action, direction, destination);
}

void BcmPort::updateMirror(
    const std::optional<std::string>& swMirrorName,
    MirrorDirection direction,
    cfg::SampleDestination sampleDest) {
  applyMirrorAction(MirrorAction::STOP, direction, sampleDest_);
  if (direction == MirrorDirection::INGRESS) {
    ingressMirror_ = swMirrorName;
  } else {
    egressMirror_ = swMirrorName;
  }
  configureSampleDestination(sampleDest);
  applyMirrorAction(MirrorAction::START, direction, sampleDest_);
}

void BcmPort::setIngressPortMirror(const std::string& mirrorName) {
  ingressMirror_ = mirrorName;
}

void BcmPort::setEgressPortMirror(const std::string& mirrorName) {
  egressMirror_ = mirrorName;
}

cfg::PortFlowletConfig BcmPort::getPortFlowletConfig() const {
  cfg::PortFlowletConfig flowletConfig;
  auto port = getProgrammedSettings();
  if (port && (port->getFlowletConfigName().has_value())) {
    if (port->getPortFlowletConfig().has_value()) {
      auto flowletCfgPtr = port->getPortFlowletConfig().value();
      CHECK(flowletCfgPtr != nullptr);
      flowletConfig.scalingFactor() = flowletCfgPtr->getScalingFactor();
      flowletConfig.loadWeight() = flowletCfgPtr->getLoadWeight();
      flowletConfig.queueWeight() = flowletCfgPtr->getQueueWeight();
    }
  }
  return flowletConfig;
}

void BcmPort::destroyAllPortStatsLocked(
    const BcmPort::PortStatsWLockedPtr& lockedPortStatsPtr) {
  std::map<std::string, stats::MonotonicCounter> swapTo;
  portCounters_.swap(swapTo);

  for (auto& item : swapTo) {
    utility::deleteCounter(item.second.getName());
  }
  queueManager_->destroyQueueCounters();

  *lockedPortStatsPtr = std::nullopt;
}

void BcmPort::enableStatCollection(const std::shared_ptr<Port>& port) {
  auto lockedPortStatsPtr = portStats_.wlock();

  XLOG(DBG2) << "Enabling stats for " << port->getName();
  if (isEnabledInSDK()) {
    XLOG(DBG2) << "Skipping bcm_port_stat_enable_set on already enabled port";
  } else {
    if (!hw_->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_QUEUE_FLEX_COUNTER)) {
      // TODO: we discovered a resource leak on this call when it
      // returns BCM_E_EXISTS so we only call on disabled ports. We
      // should remove this isEnabledInSDK() check once we confirm the api is
      // safe to use, or we get a bcm_port_stat_enable_get() api from
      // broadcom.
      //
      // Enable packet and byte counter statistic collection.
      auto rv = bcm_port_stat_enable_set(unit_, gport_, 1);
      if (rv != BCM_E_EXISTS) {
        // Don't throw an error if counter collection is already enabled
        // or this API is deprecated, e.g. on TH4
        bcmCheckError(
            rv, "Unexpected error enabling counter DMA on port ", port_);
      }
    }

    if (auto* flexCounterMgr = hw_->getBcmEgressQueueFlexCounterManager()) {
      flexCounterMgr->attachToPort(gport_);
    }

    if (hw_->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::FEC_DIAG_COUNTERS)) {
      fdrStatConfigure(unit_, port_, true);
    }
  }

  reinitPortStatsLocked(lockedPortStatsPtr, port);

  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::FEC_DIAG_COUNTERS)) {
    reinitPortFdrStats(port);
  }
}

void BcmPort::disableStatCollection() {
  auto lockedPortStatsPtr = portStats_.wlock();
  XLOG(DBG2) << "disabling stats for " << getPortName();

  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::EGRESS_QUEUE_FLEX_COUNTER)) {
    // Disable packet and byte counter statistic collection.
    auto rv = bcm_port_stat_enable_set(unit_, gport_, 0);
    bcmCheckError(rv, "Unexpected error disabling counter DMA on port ", port_);
  }

  if (auto* flexCounterMgr = hw_->getBcmEgressQueueFlexCounterManager()) {
    flexCounterMgr->detachFromPort(gport_);
  }

  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::FEC_DIAG_COUNTERS)) {
    fdrStatConfigure(unit_, port_, false);
    fdrStats_.clear();
  }

  destroyAllPortStatsLocked(lockedPortStatsPtr);
}

// Set and disable sFlow Rates.  These need to be here because the bcm function
// is not open sourced yet.
// CS8721100 setting sflow rates must come before setting sample destination.
// setting an ingress rate of 0 after setting sample destination will clear
// the sample destination.
void BcmPort::setSflowRates(const std::shared_ptr<Port>& swPort) {
  if (!swPort->getSflowIngressRate() && !swPort->getSflowEgressRate()) {
    disableSflow();
    return;
  }
  auto rv = bcm_port_sample_rate_set(
      unit_,
      port_,
      swPort->getSflowIngressRate(),
      swPort->getSflowEgressRate());
  bcmCheckError(
      rv,
      folly::sformat(
          "Failed to configure sFlow rate for unit:port {}:{}. Error: '{}' ",
          unit_,
          port_,
          bcm_errmsg(rv)));

  XLOG(DBG1) << folly::sformat(
      "Enabled sFlow for unit:port {}:{} ingress rate {} egress rate {}",
      unit_,
      port_,
      swPort->getSflowIngressRate(),
      swPort->getSflowEgressRate());
}

void BcmPort::disableSflow() {
  auto rv = bcm_port_sample_rate_set(unit_, port_, 0, 0);
  bcmCheckError(rv, "Failed to disable sFlow. Error ", bcm_errmsg(rv));

  XLOG(DBG1) << folly::sformat(
      "Disabled sFlow for unit:port {}:{}", unit_, port_);

  bcmCheckError(
      rv, "Failed to disable sFlow port control. Error ", bcm_errmsg(rv));
}

void BcmPort::prepareForGracefulExit() {
  platformPort_->prepareForGracefulExit();
}

uint32_t BcmPort::getCL91FECStatus() const {
  uint32_t cl91Status{0};
  auto rv = bcm_port_phy_control_get(
      unit_,
      port_,
      BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91,
      &cl91Status);
  if (rv == BCM_E_UNAVAIL) {
    XLOG(DBG2) << "Failed to read if CL91 FEC is enabled: " << bcm_errmsg(rv);
    return 0;
  }
  bcmCheckError(rv, "failed to read if CL91 FEC is enabled");
  return cl91Status;
}

bool BcmPort::isCL91FECApplicable() const {
  auto asic = hw_->getPlatform()->getAsic()->getAsicType();
  return asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK ||
      asic == cfg::AsicType::ASIC_TYPE_FAKE;
}

bool BcmPort::setFEC(const std::shared_ptr<Port>& swPort) {
  bool changed = false;

  // Change FEC setting on an UP port could cause port flap.
  // Only do it when the port is not UP.
  //
  // We are extra conservative here due to bcm case #1134023, where we
  // cannot rely on the value of bcm_port_phy_control_get for
  // BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION on 6.4.10
  if (isUp()) {
    XLOG(DBG1) << "Skip FEC setting on port " << swPort->getID()
               << ", which is UP";
    return changed;
  }

  const auto desiredFecMode = *swPort->getProfileConfig().fec();
  bool shouldEnableFec = !platformPort_->shouldDisableFEC() &&
      desiredFecMode != phy::FecMode::NONE;
  auto desiredFecStatus = shouldEnableFec ? BCM_PORT_PHY_CONTROL_FEC_ON
                                          : BCM_PORT_PHY_CONTROL_FEC_OFF;

  uint32_t currentFecStatus{0};
  auto rv = bcm_port_phy_control_get(
      unit_,
      port_,
      BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION,
      &currentFecStatus);
  bcmCheckError(rv, "failed to read if FEC is enabled");

  XLOG(DBG1) << "FEC for port " << swPort->getID() << " is "
             << (currentFecStatus == BCM_PORT_PHY_CONTROL_FEC_ON ? "ON"
                                                                 : "OFF");

  if (desiredFecStatus != currentFecStatus) {
    XLOG(DBG1) << (shouldEnableFec ? "Enabling" : "Disabling")
               << " FEC on port " << swPort->getID();
    rv = bcm_port_phy_control_set(
        unit_,
        port_,
        BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION,
        desiredFecStatus);
    bcmCheckError(rv, "failed to enable/disable fec");
    changed = true;
  }

  int desiredCl91Status =
      (shouldEnableFec && desiredFecMode == phy::FecMode::CL91) ? 1 : 0;

  auto cl91Status = getCL91FECStatus();
  if (desiredCl91Status != cl91Status) {
    XLOG(DBG1) << "Turning CL91 " << (desiredCl91Status ? "on" : "off")
               << " for port " << swPort->getID();
    rv = bcm_port_phy_control_set(
        unit_,
        port_,
        BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91,
        desiredCl91Status);
    if (rv == BCM_E_UNAVAIL) {
      XLOG(DBG2) << "Failed to set CL91 FEC is enabled: " << bcm_errmsg(rv);
      return changed;
    }
    bcmCheckError(rv, "failed to set cl91 fec setting");
    changed = true;
  }

  return changed;
}

phy::TxSettings BcmPort::getTxSettingsForLane(int lane) const {
  // lane number is relative, so we can use the port resource api directly
  if (platformPort_->shouldUsePortResourceAPIs()) {
    bcm_gport_t gport;
    BCM_PHY_GPORT_LANE_PORT_SET(gport, lane, port_);

    bcm_port_phy_tx_t tx;
    bcm_port_phy_tx_t_init(&tx);

    auto rv = bcm_port_phy_tx_get(unit_, gport, &tx);
    bcmCheckError(
        rv, "Unable to get tx settings on lane ", lane, " for port ", port_);
    phy::TxSettings txSettings;
    txSettings.pre2() = tx.pre2;
    txSettings.pre() = tx.pre;
    txSettings.main() = tx.main;
    txSettings.post() = tx.post;
    txSettings.post2() = tx.post2;
    txSettings.post3() = tx.post3;
    return txSettings;
  } else {
    uint32_t dc, preTap, mainTap, postTap;
    auto rv = bcm_port_phy_control_get(
        unit_, port_, BCM_PORT_PHY_CONTROL_DRIVER_CURRENT, &dc);
    bcmCheckError(rv, "failed to get driver current");
    auto pre = bcm_port_phy_control_get(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_PRE, &preTap);
    bcmCheckError(pre, "failed to get pre-tap");
    auto main = bcm_port_phy_control_get(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_MAIN, &mainTap);
    bcmCheckError(main, "failed to get main-tap");
    auto post = bcm_port_phy_control_get(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_POST, &postTap);
    bcmCheckError(post, "failed to get post-tap");
    phy::TxSettings txSettings;
    txSettings.driveCurrent() = dc;
    txSettings.pre() = preTap;
    txSettings.main() = mainTap;
    txSettings.post() = postTap;
    return txSettings;
  }
}

void BcmPort::setTxSetting(const std::shared_ptr<Port>& swPort) {
  const auto& iphyPinConfigs = swPort->getPinConfigs();
  if (iphyPinConfigs.empty()) {
    XLOG(DBG2) << "No iphy pin configs to program for " << swPort->getName();
    return;
  }
  if (platformPort_->shouldUsePortResourceAPIs()) {
    setTxSettingViaPhyTx(swPort, iphyPinConfigs);
  } else {
    setTxSettingViaPhyControl(swPort, iphyPinConfigs);
  }
}

// Set the preemphasis (pre-tap; main-tap; post-tap) setting for the transmitter
// and the amplitude control (driver current) values
void BcmPort::setTxSettingViaPhyControl(
    const std::shared_ptr<Port>& swPort,
    const std::vector<phy::PinConfig>& iphyPinConfigs) {
  // Changing the preemphasis/driver-current setting won't cause link flap,
  // so it's safe to not check for port status

  // All the pins use the same config so just use the first one
  const auto tx = iphyPinConfigs.front().tx();
  if (!tx.has_value()) {
    // If TxSetting returns no values, it means that we don't need to update
    // tx setting for the port. So do nothing
    return;
  }
  const auto correctTx = tx.value();
  uint32_t dc;
  uint32_t preTap;
  uint32_t mainTap;
  uint32_t postTap;

  // Get current Tx Setting used. Register names can be found on page 20 of
  // broadcom debug document
  auto rv = bcm_port_phy_control_get(
      unit_, port_, BCM_PORT_PHY_CONTROL_DRIVER_CURRENT, &dc);
  bcmCheckError(rv, "failed to get driver current");
  auto pre = bcm_port_phy_control_get(
      unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_PRE, &preTap);
  bcmCheckError(pre, "failed to get pre-tap");
  auto main = bcm_port_phy_control_get(
      unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_MAIN, &mainTap);
  bcmCheckError(main, "failed to get main-tap");
  auto post = bcm_port_phy_control_get(
      unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_POST, &postTap);
  bcmCheckError(post, "failed to get post-tap");

  XLOG(DBG4) << "Drive current on port " << swPort->getID() << " is " << dc;
  XLOG(DBG4) << "Pre-tap on port " << swPort->getID() << " is " << preTap;
  XLOG(DBG4) << "Main-tap on port " << swPort->getID() << " is " << mainTap;
  XLOG(DBG4) << "Post-tap on port " << swPort->getID() << " is " << postTap;

  // only perform an overwrite if the current setting doesn't match the
  // current one
  if (auto correctDc = correctTx.driveCurrent()) {
    // const auto correctDc =
    //     static_cast<uint32_t>(correctTx.driveCurrent_ref().value());
    if (dc != correctDc && correctDc != 0) {
      bcm_port_phy_control_set(
          unit_, port_, BCM_PORT_PHY_CONTROL_DRIVER_CURRENT, *correctDc);

      XLOG(DBG1) << "Set drive current on port " << swPort->getID() << " to be "
                 << static_cast<uint32_t>(*correctDc) << " from " << dc;
    }
  }
  if (preTap != *correctTx.pre() && *correctTx.pre() != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_PRE, *correctTx.pre());
    XLOG(DBG1) << "Set pre-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(*correctTx.pre()) << " from " << preTap;
  }
  if (mainTap != *correctTx.main() && *correctTx.main() != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_MAIN, *correctTx.main());
    XLOG(DBG1) << "Set main-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(*correctTx.main()) << " from "
               << mainTap;
  }
  if (postTap != *correctTx.post() && *correctTx.post() != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_POST, *correctTx.post());
    XLOG(DBG1) << "Set post-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(*correctTx.post()) << " from "
               << postTap;
  }
}

void BcmPort::setTxSettingViaPhyTx(
    const std::shared_ptr<Port>& swPort,
    const std::vector<phy::PinConfig>& iphyPinConfigs) {
  auto profileID = swPort->getProfileID();
  if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
    XLOG(WARNING)
        << swPort->getName()
        << " has default profile, skip tx_setting programming for now";
    return;
  }

  const auto& iphyLaneConfigs = utility::getIphyLaneConfigs(iphyPinConfigs);
  if (iphyLaneConfigs.empty()) {
    XLOG(DBG2) << "No iphy lane configs needed to program for "
               << swPort->getName();
    return;
  }

  auto portProfileConfig = swPort->getProfileConfig();
  auto modulation = *portProfileConfig.modulation();
  auto desiredSignalling = (modulation == phy::IpModulation::PAM4)
      ? bcmPortPhySignallingModePAM4
      : bcmPortPhySignallingModeNRZ;

  // We currently pass the absolute lane index through the
  // config. Based on experimentation there was a non-backwards
  // compatible change between 6.5.14 and 6.5.16. In 6.5.16 the
  // phy apis expect lanes starting at 0 up to number of lanes in
  // a port. In 6.5.14, they expect the exact lane number
  int minLane = -1;
  for (const auto& it : iphyLaneConfigs) {
    if (minLane < 0 || it.first < minLane) {
      minLane = it.first;
    }
  }

  bool needsReprogramming{false};
  for (const auto& it : iphyLaneConfigs) {
    auto lane = it.first - minLane;
    auto txSettings = it.second.tx;

    if (!txSettings.has_value()) {
      throw FbossError(
          "Missing tx for port ", swPort->getName(), " lane ", lane);
    }

    bcm_gport_t gport;
    BCM_PHY_GPORT_LANE_PORT_SET(gport, lane, port_);

    bcm_port_phy_tx_t tx;
    bcm_port_phy_tx_t_init(&tx);

    auto rv = bcm_port_phy_tx_get(unit_, gport, &tx);
    bcmCheckError(
        rv,
        "Unable to get tx settings on lane ",
        lane,
        " for ",
        swPort->getName());

    if (desiredSignalling != tx.signalling_mode ||
        tx.pre2 != *txSettings->pre2() || tx.pre != *txSettings->pre() ||
        tx.main != *txSettings->main() || tx.post != *txSettings->post() ||
        tx.post2 != *txSettings->post2() || tx.post3 != *txSettings->post3()) {
      // settings don't match, reprogram all lanes
      needsReprogramming = true;
      break;
    }
  }

  if (!needsReprogramming) {
    return;
  }

  for (const auto& it : iphyLaneConfigs) {
    auto lane = it.first - minLane;
    auto txSettings = it.second.tx;

    bcm_gport_t gport;
    BCM_PHY_GPORT_LANE_PORT_SET(gport, lane, port_);

    bcm_port_phy_tx_t tx;
    bcm_port_phy_tx_t_init(&tx);

    tx.pre2 = *txSettings->pre2();
    tx.pre = *txSettings->pre();
    tx.main = *txSettings->main();
    tx.post = *txSettings->post();
    tx.post2 = *txSettings->post2();
    tx.post3 = *txSettings->post3();
    tx.tx_tap_mode = bcmPortPhyTxTapMode6Tap;
    tx.signalling_mode = desiredSignalling;

    auto rv = bcm_port_phy_tx_set(unit_, gport, &tx);
    bcmCheckError(
        rv,
        "Unable to set tx settings on lane ",
        lane,
        " for ",
        swPort->getName());
  }
}

void BcmPort::setLoopbackMode(cfg::PortLoopbackMode mode) {
  int newLoopbackMode = utility::fbToBcmLoopbackMode(mode);
  int oldLoopbackMode;
  auto rv = bcm_port_loopback_get(unit_, port_, &oldLoopbackMode);
  bcmCheckError(rv, "failed to get loopback mode state for port", port_);
  if (oldLoopbackMode != newLoopbackMode) {
    rv = bcm_port_loopback_set(unit_, port_, newLoopbackMode);
    bcmCheckError(
        rv,
        "failed to set loopback mode state to ",
        apache::thrift::util::enumNameSafe(mode),
        " for port",
        port_);
  }
}

phy::FecMode BcmPort::getFECMode() const {
  if (platformPort_->shouldUsePortResourceAPIs()) {
    auto curPortResource = getCurrentPortResource(unit_, gport_);
    return utility::bcmPortPhyFecToPhyFecMode(curPortResource.fec_type);
  } else {
    if (isCL91FECApplicable() && getCL91FECStatus()) {
      return phy::FecMode::CL91;
    } else {
      uint32_t value = BCM_PORT_PHY_CONTROL_FEC_OFF;
      auto rv = bcm_port_phy_control_get(
          unit_, port_, BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION, &value);
      bcmCheckError(rv, "failed to read if FEC is enabled");
      // compare against 'OFF' because result could be 'ON'
      // or 'AUTO'  - not clear how to verify AUTO has gone to yes
      return value != BCM_PORT_PHY_CONTROL_FEC_OFF ? phy::FecMode::CL74
                                                   : phy::FecMode::NONE;
    }
  }
}

bool BcmPort::getFdrEnabled() const {
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::FEC_DIAG_COUNTERS)) {
    return false;
  }

  bcm_port_fdr_config_t fdr_config;

  bcmCheckError(
      bcm_port_fdr_config_get(unit_, port_, &fdr_config),
      "Failed to fetch current error telemetry config for unit ",
      unit_,
      " port ",
      port_);

  return fdr_config.fdr_enable;
#else
  return false;
#endif
}

void BcmPort::initCustomStats() const {
  int rv = 0;
  auto asicType = hw_->getPlatform()->getAsic()->getAsicType();
  if (asicType == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
      asicType == cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
    rv = bcm_stat_custom_add(
        0, port_, snmpBcmCustomReceive3, bcmDbgCntRxL3DstDiscardDrop);
  } else {
    rv = bcm_stat_custom_add(
        0, port_, snmpBcmCustomReceive3, bcmDbgCntDSTDISCARDDROP);
  }
  bcmCheckError(rv, "Unable to set up custom stat for DST discard drops");
}

bcm_gport_t BcmPort::asGPort(bcm_port_t port) {
  bcm_gport_t rtn;
  BCM_GPORT_LOCAL_SET(rtn, port);
  return rtn;
}

bool BcmPort::isValidLocalPort(bcm_gport_t gport) {
  return BCM_GPORT_IS_LOCAL(gport) &&
      BCM_GPORT_LOCAL_GET(gport) != static_cast<bcm_port_t>(0);
}

void BcmPort::getPfcCosqDeadlockControl(
    const int pri,
    const bcm_cosq_pfc_deadlock_control_t control,
    int* value,
    const std::string& controlStr) {
  auto rv =
      bcm_cosq_pfc_deadlock_control_get(unit_, port_, pri, control, value);
  bcmCheckError(
      rv, "Failed to get ", controlStr, " for port  ", port_, " cosq ", pri);
}

void BcmPort::getProgrammedPfcWatchdogParams(
    const int pri,
    std::map<bcm_cosq_pfc_deadlock_control_t, int>& pfcWatchdogControls) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }

  int value;
  // Make sure the watchdog controls are cleared
  pfcWatchdogControls.clear();
  getPfcCosqDeadlockControl(
      pri,
      bcmCosqPFCDeadlockTimerGranularity,
      &value,
      "bcmCosqPFCDeadlockTimerGranularity");
  pfcWatchdogControls[bcmCosqPFCDeadlockTimerGranularity] = value;

  getPfcCosqDeadlockControl(
      pri,
      bcmCosqPFCDeadlockDetectionTimer,
      &value,
      "bcmCosqPFCDeadlockDetectionTimer");
  pfcWatchdogControls[bcmCosqPFCDeadlockDetectionTimer] = value;

  getPfcCosqDeadlockControl(
      pri,
      bcmCosqPFCDeadlockRecoveryTimer,
      &value,
      "bcmCosqPFCDeadlockRecoveryTimer");
  pfcWatchdogControls[bcmCosqPFCDeadlockRecoveryTimer] = value;

  getPfcCosqDeadlockControl(
      pri,
      bcmCosqPFCDeadlockDetectionAndRecoveryEnable,
      &value,
      "bcmCosqPFCDeadlockDetectionAndRecoveryEnable");
  pfcWatchdogControls[bcmCosqPFCDeadlockDetectionAndRecoveryEnable] = value;
}

std::vector<PfcPriority> BcmPort::getLastConfiguredPfcPriorities() {
  std::vector<PfcPriority> enabledPfcPriorities;
  auto savedPort = getProgrammedSettings();
  if (savedPort) {
    for (auto pfcPri : savedPort->getPfcPriorities()) {
      enabledPfcPriorities.push_back(pfcPri);
    }
  }
  return enabledPfcPriorities;
}

bool BcmPort::pfcWatchdogNeedsReprogramming(const std::shared_ptr<Port>& port) {
  int pfcWatchdogEnabledInSw = 0;
  int newPfcDeadlockRecoveryTimer{};
  int newPfcDeadlockDetectionTimer{};
  std::map<bcm_cosq_pfc_deadlock_control_t, int> programmedPfcWatchdogMap;

  if (port->getPfc().has_value() && port->getPfc()->watchdog().has_value()) {
    auto pfcWatchdog = port->getPfc()->watchdog().value();
    newPfcDeadlockDetectionTimer =
        utility::getAdjustedPfcDeadlockDetectionTimerValue(
            *pfcWatchdog.detectionTimeMsecs());
    auto asicType = hw_->getPlatform()->getAsic()->getAsicType();
    newPfcDeadlockRecoveryTimer =
        utility::getAdjustedPfcDeadlockRecoveryTimerValue(
            asicType, *pfcWatchdog.recoveryTimeMsecs());
    pfcWatchdogEnabledInSw = 1;
  }

  /*
   * Assuming the programmed PFC watchdog configs are the
   * same for all enabled priorities. Also, a change in
   * enabled PFC priorites is not something that is being
   * handled here, that would need a cold boot and should
   * not hit unconfig flow.
   */
  int enabledPfcPriority = 0;
  if (!port->getPfcPriorities().empty()) {
    enabledPfcPriority = port->getPfcPriorities().at(0);
  } else {
    auto lastPfcPriorities = getLastConfiguredPfcPriorities();
    if (!lastPfcPriorities.empty()) {
      enabledPfcPriority = lastPfcPriorities[0];
    }
  }
  getProgrammedPfcWatchdogParams(enabledPfcPriority, programmedPfcWatchdogMap);

  if ((pfcWatchdogEnabledInSw !=
       programmedPfcWatchdogMap
           [bcmCosqPFCDeadlockDetectionAndRecoveryEnable]) ||
      (newPfcDeadlockDetectionTimer !=
       programmedPfcWatchdogMap[bcmCosqPFCDeadlockDetectionTimer]) ||
      (newPfcDeadlockRecoveryTimer !=
       programmedPfcWatchdogMap[bcmCosqPFCDeadlockRecoveryTimer])) {
    XLOG(DBG2) << "Currently programmed PFC watchdog params: "
               << " recoveryTimer: "
               << programmedPfcWatchdogMap[bcmCosqPFCDeadlockRecoveryTimer]
               << " detectionTimer: "
               << programmedPfcWatchdogMap[bcmCosqPFCDeadlockDetectionTimer]
               << " detectionTimerGranularity: "
               << programmedPfcWatchdogMap[bcmCosqPFCDeadlockTimerGranularity]
               << " detectionAndRecoveryEnable: "
               << programmedPfcWatchdogMap
                      [bcmCosqPFCDeadlockDetectionAndRecoveryEnable];
    XLOG(DBG2) << "New PFC watchdog params: "
               << " recoveryTimer: " << newPfcDeadlockRecoveryTimer
               << " detectionTimer: " << newPfcDeadlockDetectionTimer
               << " detectionAndRecoveryEnable: " << pfcWatchdogEnabledInSw;
    // Mismatch between SW and HW configs, reprogram!
    return true;
  }

  // XXX: Handle the case where PFC enabled priorities change once supported!
  return false;
}

void BcmPort::setPfcCosqDeadlockControl(
    const int pri,
    const bcm_cosq_pfc_deadlock_control_t control,
    const int value,
    const std::string& controlStr) {
  XLOG(DBG2) << "Set PFC deadlock control " << controlStr << " with " << value
             << " for port " << port_ << " cosq " << pri;
  auto rv =
      bcm_cosq_pfc_deadlock_control_set(unit_, port_, pri, control, value);
  bcmCheckError(
      rv, "Failed to set ", controlStr, " for port  ", port_, " cosq ", pri);
}

void BcmPort::programPfcWatchdog(const std::shared_ptr<Port>& swPort) {
  // Initialize to default params which has PFC watchdog disabled
  auto asicType = hw_->getPlatform()->getAsic()->getAsicType();
  int pfcDeadlockRecoveryTimer =
      utility::getDefaultPfcDeadlockRecoveryTimer(asicType);
  int pfcDeadlockRecoveryAction{bcmSwitchPFCDeadlockActionTransmit};
  int pfcDeadlockDetectionTimer =
      utility::getDefaultPfcDeadlockDetectionTimer(asicType);
  int pfcDeadlockTimerGranularity{bcmCosqPFCDeadlockTimerInterval1MiliSecond};
  int pfcDeadlockDetectionAndRecoveryEnable{0};

  auto programPerPriorityPfcWatchdog = [&](int pri) {
    // XXX: Work around in CS00012182388 for PFC WD disable
    if (!pfcDeadlockDetectionAndRecoveryEnable) {
      setPfcCosqDeadlockControl(
          pri,
          bcmCosqPFCDeadlockDetectionAndRecoveryEnable,
          pfcDeadlockDetectionAndRecoveryEnable,
          "bcmCosqPFCDeadlockDetectionAndRecoveryEnable");
    }

    // TimerGranularity needs to be set before DetectionTimer
    setPfcCosqDeadlockControl(
        pri,
        bcmCosqPFCDeadlockTimerGranularity,
        pfcDeadlockTimerGranularity,
        "bcmCosqPFCDeadlockTimerGranularity");
    setPfcCosqDeadlockControl(
        pri,
        bcmCosqPFCDeadlockDetectionTimer,
        pfcDeadlockDetectionTimer,
        "bcmCosqPFCDeadlockDetectionTimer");
    setPfcCosqDeadlockControl(
        pri,
        bcmCosqPFCDeadlockRecoveryTimer,
        pfcDeadlockRecoveryTimer,
        "bcmCosqPFCDeadlockRecoveryTimer");

    // XXX: Work around in CS00012182388 for PFC WD enable
    if (pfcDeadlockDetectionAndRecoveryEnable) {
      setPfcCosqDeadlockControl(
          pri,
          bcmCosqPFCDeadlockDetectionAndRecoveryEnable,
          pfcDeadlockDetectionAndRecoveryEnable,
          "bcmCosqPFCDeadlockDetectionAndRecoveryEnable");
    }
  };

  auto populatePfcWatchdogParams = [&](const cfg::PfcWatchdog& pfcWd) {
    pfcDeadlockDetectionAndRecoveryEnable = 1;
    pfcDeadlockDetectionTimer = *pfcWd.detectionTimeMsecs();
    if (pfcDeadlockDetectionTimer > kPfcDeadlockDetectionTimeMsecMax) {
      XLOG(WARNING) << "Unsupported PFC deadlock detection timer value "
                    << pfcDeadlockDetectionTimer
                    << " configured, truncating to maximum possible value "
                    << kPfcDeadlockDetectionTimeMsecMax;
      pfcDeadlockDetectionTimer = kPfcDeadlockDetectionTimeMsecMax;
    } else if (pfcDeadlockDetectionTimer == 0) {
      // XXX: Workaround for CS00012182388, non zero value expected
      XLOG(WARNING) << "Unsupported PFC deadlock detection timer value "
                    << pfcDeadlockDetectionTimer
                    << " configured, setting to the min possible value 1msec";
      pfcDeadlockDetectionTimer = 1;
    }
    pfcDeadlockTimerGranularity =
        utility::getBcmPfcDeadlockDetectionTimerGranularity(
            pfcDeadlockDetectionTimer);
    pfcDeadlockRecoveryTimer =
        utility::getAdjustedPfcDeadlockRecoveryTimerValue(
            asicType, *pfcWd.recoveryTimeMsecs());

    switch (*pfcWd.recoveryAction()) {
      case cfg::PfcWatchdogRecoveryAction::DROP:
        pfcDeadlockRecoveryAction = bcmSwitchPFCDeadlockActionDrop;
        break;
      case cfg::PfcWatchdogRecoveryAction::NO_DROP:
      default:
        pfcDeadlockRecoveryAction = bcmSwitchPFCDeadlockActionTransmit;
        break;
    }
  };

  std::vector<PfcPriority> enabledPfcPriorities;
  if (swPort->getPfc().has_value() &&
      swPort->getPfc()->watchdog().has_value()) {
    populatePfcWatchdogParams(swPort->getPfc()->watchdog().value());
    for (const auto& pfcPri : swPort->getPfcPriorities()) {
      enabledPfcPriorities.push_back(static_cast<PfcPriority>(pfcPri));
    }
  } else {
    // Default values initialized will be used, expected for unconfig cases.
    enabledPfcPriorities = getLastConfiguredPfcPriorities();
  }

  XLOG(DBG2) << "PFC watchdog being programmed, port: " << port_
             << " recoveryTimer: " << pfcDeadlockRecoveryTimer
             << " recoveryAction: " << pfcDeadlockRecoveryAction
             << " detectionTimer: " << pfcDeadlockDetectionTimer
             << " timerGranularity: " << pfcDeadlockTimerGranularity
             << " detectionAndRecoveryEnable: "
             << pfcDeadlockDetectionAndRecoveryEnable;

  // Program per priority PFC watchdog configurations
  for (auto pri : enabledPfcPriorities) {
    programPerPriorityPfcWatchdog(pri);
  }
}

void BcmPort::getProgrammedPfcState(int* pfcRx, int* pfcTx) {
  auto rv = bcm_port_control_get(unit_, port_, bcmPortControlPFCReceive, pfcRx);
  bcmCheckError(rv, "Failed to read pfcRx for port ", port_);

  rv = bcm_port_control_get(unit_, port_, bcmPortControlPFCTransmit, pfcTx);
  bcmCheckError(rv, "Failed to read pfcTx for port ", port_);
}

void BcmPort::programPfc(const int enableTxPfc, const int enableRxPfc) {
  int rv = 0;
  if (enableTxPfc || enableRxPfc) {
    int currTxPause = 0;
    int currRxPause = 0;
    rv = bcm_port_pause_get(unit_, port_, &currTxPause, &currRxPause);
    bcmCheckError(
        rv,
        "failed to retrieve rx/tx pause using bcm_port_pause_get for port ",
        port_);

    // don't expect pause to be enabled at the same time as PFC here
    // since order of programming is pause followed by PFC
    CHECK((currTxPause == 0) && (currRxPause == 0))
        << "PAUSE cannot be enabled when PFC is enabled for:" << port_;
  }

  rv = bcm_port_control_set(
      unit_, port_, bcmPortControlPFCTransmit, enableTxPfc);
  bcmCheckError(rv, "failed to set bcmPortControlPFCTransmit for port ", port_);

  rv =
      bcm_port_control_set(unit_, port_, bcmPortControlPFCReceive, enableRxPfc);
  bcmCheckError(rv, "failed to set bcmPortControlPFCReceive for port ", port_);

  auto logHelper = [](int tx, int rx) {
    return folly::to<std::string>(
        tx ? "True/" : "False/", rx ? "True" : "False");
  };

  XLOG(DBG3) << "Successfully enabled pfc on port: " << port_
             << " , TX/RX = " << logHelper(enableTxPfc, enableRxPfc);
}

void BcmPort::setCosqProfile(const int profileId) {
  int curProfileId;
  int rv = bcm_cosq_port_profile_get(
      unit_, port_, bcmCosqProfileIntPriToPGMap, &curProfileId);
  bcmCheckError(
      rv, "failed to get bcmCosqProfileIntPriToPGMap for port ", port_);
  if (profileId == curProfileId) {
    XLOG(DBG3)
        << "port " << port_
        << " is already programmed with bcmCosqProfileIntPriToPGMap profile id: "
        << profileId;
  } else {
    XLOG(DBG3) << "program port " << port_
               << " to use new bcmCosqProfileIntPriToPGMap profile id "
               << profileId << " from current profile id " << curProfileId;
    rv = bcm_cosq_port_profile_set(
        unit_, port_, bcmCosqProfileIntPriToPGMap, profileId);
    bcmCheckError(
        rv, "failed to set bcmCosqProfileIntPriToPGMap for port ", port_);
  }
}

void BcmPort::setPfc(const std::shared_ptr<Port>& swPort) {
  auto pfc = swPort->getPfc();
  int expectTxPfc = 0;
  int expectRxPfc = 0;
  bool pfcConfigChanged = false;

  if (pfc.has_value()) {
    expectTxPfc = (*pfc->tx()) ? 1 : 0;
    expectRxPfc = (*pfc->rx()) ? 1 : 0;
  }

  int currTxPfcEnabled = 0;
  int currRxPfcEnabled = 0;

  auto rv = bcm_port_control_get(
      unit_, port_, bcmPortControlPFCTransmit, &currTxPfcEnabled);
  bcmCheckError(rv, "failed to read pfcTx for port ", port_);

  rv = bcm_port_control_get(
      unit_, port_, bcmPortControlPFCReceive, &currRxPfcEnabled);
  bcmCheckError(rv, "failed to read pfcRx for port ", port_);

  auto logHelper = [](int tx, int rx) {
    return folly::to<std::string>(
        tx ? "True/" : "False/", rx ? "True" : "False");
  };

  if ((expectTxPfc == currTxPfcEnabled) && (expectRxPfc == currRxPfcEnabled)) {
    XLOG(DBG4) << "Skip same PFC setting for port " << port_
               << ", Tx/Rx =" << logHelper(expectTxPfc, expectRxPfc);
  } else {
    pfcConfigChanged = true;
    // enable/disable pfc
    programPfc(expectTxPfc, expectRxPfc);
  }

  if (!pfcConfigChanged && currTxPfcEnabled == 0 && currRxPfcEnabled == 0) {
    /*
     * If PFC config has not changed and PFC is not currently enabled,
     * there is no need to try to program PFC watchdog, return!
     */
    return;
  }

  if (pfcWatchdogNeedsReprogramming(swPort)) {
    // Enable/disable pfc watchdog
    programPfcWatchdog(swPort);
  }
}

void BcmPort::setPause(const std::shared_ptr<Port>& swPort) {
  auto pause = swPort->getPause();
  int expectTx = (*pause.tx()) ? 1 : 0;
  int expectRx = (*pause.rx()) ? 1 : 0;

  int curTx = 0;
  int curRx = 0;
  auto rv = bcm_port_pause_get(unit_, port_, &curTx, &curRx);
  bcmCheckError(rv, "failed to get pause setting from HW for port ", port_);

  auto logHelper = [](int tx, int rx) {
    return folly::to<std::string>(
        tx ? "True/" : "False/", rx ? "True" : "False");
  };

  if (expectTx == curTx && expectRx == curRx) {
    // nothing to do
    XLOG(DBG4) << "Skip same pause setting for port " << port_
               << ", Tx/Rx==" << logHelper(expectTx, expectRx);
    return;
  }

  rv = bcm_port_pause_set(unit_, port_, expectTx, expectRx);
  bcmCheckError(
      rv,
      "failed to set pause setting for port ",
      port_,
      ", Tx/Rx=>",
      logHelper(expectTx, expectRx));

  XLOG(DBG1) << "set pause setting for port " << port_ << ", Tx/Rx=>"
             << logHelper(expectTx, expectRx);
}

QueueConfig BcmPort::getCurrentQueueSettings() {
  // BcmPortQueueManager will return both unicast and multicast queue settings.
  // But for now, BcmPort only cares unicast
  return queueManager_->getCurrentQueueSettings().unicast;
}

void BcmPort::setupQueue(const PortQueue& queue) {
  queueManager_->program(queue);
}

void BcmPort::attachIngressQosPolicy(const std::string& name) {
  /*
   * In practice, we noticed that below call to bcm_qos_port_map_set had a
   * side-effect: it  set dscp map mode to BCM_PORT_DSCP_MAP_ALL.
   * However, this is not documented, and Broadcom recommended explicitly
   * calling bcm_port_dscp_map_mode_set.
   */
  auto rv =
      bcm_port_dscp_map_mode_set(hw_->getUnit(), port_, BCM_PORT_DSCP_MAP_ALL);
  bcmCheckError(rv, "failed to set dscp map mode");

  auto qosPolicy = hw_->getQosPolicyTable()->getQosPolicy(name);
  rv = bcm_qos_port_map_set(
      hw_->getUnit(),
      gport_,
      qosPolicy->getHandle(BcmQosMap::Type::IP_INGRESS),
      -1);
  bcmCheckError(rv, "failed to set qos map");
}

void BcmPort::detachIngressQosPolicy() {
  auto rv = bcm_qos_port_map_set(hw_->getUnit(), gport_, 0, 0);
  bcmCheckError(rv, "failed to unset qos map");
}

void BcmPort::destroy() {
  auto destroyed = destroyed_.exchange(true);
  if (!destroyed) {
    applyMirrorAction(
        MirrorAction::STOP, MirrorDirection::INGRESS, sampleDest_);
    applyMirrorAction(MirrorAction::STOP, MirrorDirection::EGRESS, sampleDest_);
  }
}

bool BcmPort::setPortResource(const std::shared_ptr<Port>& swPort) {
  bool changed = false;

  // Since BRCM SDK 6.5.13, there're a lot of port apis deprecated for PM8x50
  // ports(like TH3). And a new powerful, atomic api bcm_port_resource_get/set
  // is created. Therefore, we're able to configure different attributes for a
  // bcm_port_resource, e.g. speed, fec_type and etc, all together.
  auto curPortResource = getCurrentPortResource(unit_, gport_);
  auto desiredPortResource = curPortResource;

  auto profileID = swPort->getProfileID();
  const auto& profileConf = swPort->getProfileConfig();

  XLOG(DBG1) << "Program port resource based on speed profile: "
             << apache::thrift::util::enumNameSafe(profileID);
  desiredPortResource.speed = static_cast<int>(swPort->getSpeed());
  desiredPortResource.fec_type = utility::phyFecModeToBcmPortPhyFec(
      platformPort_->shouldDisableFEC() ? phy::FecMode::NONE
                                        : *profileConf.fec());
  desiredPortResource.phy_lane_config =
      utility::getDesiredPhyLaneConfig(profileConf);

  auto isPortResourceSame = [](const bcm_port_resource_t& current,
                               const bcm_port_resource_t& desired) {
    // Right now, we only need to care the speed and fec_type and medium
    return current.speed == desired.speed &&
        current.fec_type == desired.fec_type &&
        current.phy_lane_config == desired.phy_lane_config;
  };

  if (isPortResourceSame(curPortResource, desiredPortResource)) {
    XLOG(DBG2)
        << "Desired port resource is the same as current one. No need to "
        << "reprogram it. Port: " << swPort->getName()
        << ", id: " << swPort->getID()
        << ", speed: " << desiredPortResource.speed
        << ", fec_type: " << desiredPortResource.fec_type
        << ", phy_lane_config: 0x" << std::hex
        << desiredPortResource.phy_lane_config;
    return changed;
  }

  if (isUp()) {
    XLOG(WARNING) << "Changing port resource on up port. This might "
                  << "disrupt traffic. Port: " << swPort->getName()
                  << ", id: " << swPort->getID();
  }

  XLOG(DBG1) << "Changing port resource Port " << swPort->getName() << ", id "
             << swPort->getID() << ": from speed=" << curPortResource.speed
             << ", fec_type=" << curPortResource.fec_type
             << ", phy_lane_config=0x" << std::hex
             << curPortResource.phy_lane_config << ", to speed=" << std::dec
             << desiredPortResource.speed
             << ", fec_type=" << desiredPortResource.fec_type
             << ", phy_lane_config=0x" << std::hex
             << desiredPortResource.phy_lane_config;

  auto rv = bcm_port_resource_speed_set(unit_, gport_, &desiredPortResource);
  bcmCheckError(rv, "failed to set port resource on port ", swPort->getID());
  changed = true;
  return changed;
}

cfg::PortProfileID BcmPort::getCurrentProfile() const {
  if (getHW()->getRunState() < SwitchRunState::CONFIGURED) {
    // switch is not configured yet, settings may not be programmed.
    auto profile = getPlatformPort()->getProfileIDBySpeedIf(getSpeed());
    CHECK(profile.has_value());
    return profile.value();
  }
  return (*programmedSettings_.rlock())->getProfileID();
}

bool BcmPort::isPortPgConfigured() const {
  return (
      hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC) &&
      (*programmedSettings_.rlock())->getPortPgConfigs());
}

PortPgConfigs BcmPort::getCurrentProgrammedPgSettings() const {
  return ingressBufferManager_->getCurrentProgrammedPgSettingsHw();
}

PortPgConfigs BcmPort::getCurrentPgSettings() const {
  return ingressBufferManager_->getCurrentPgSettingsHw();
}

const PortPgConfig& BcmPort::getDefaultPgSettings() const {
  return ingressBufferManager_->getDefaultPgSettings();
}

BufferPoolCfgPtr BcmPort::getCurrentIngressPoolSettings() const {
  return ingressBufferManager_->getCurrentIngressPoolSettings();
}

const BufferPoolCfg& BcmPort::getDefaultIngressPoolSettings() const {
  return ingressBufferManager_->getDefaultIngressPoolSettings();
}

int BcmPort::getProgrammedPgLosslessMode(const int pgId) const {
  return ingressBufferManager_->getProgrammedPgLosslessMode(pgId);
}

int BcmPort::getProgrammedPfcStatusInPg(const int pgId) const {
  return ingressBufferManager_->getProgrammedPfcStatusInPg(pgId);
}

int BcmPort::getPgMinLimitBytes(const int pgId) const {
  return ingressBufferManager_->getPgMinLimitBytes(pgId);
}

int BcmPort::getIngressSharedBytes(const int pgId) const {
  return ingressBufferManager_->getIngressSharedBytes(pgId);
}

uint32_t BcmPort::getInterPacketGapBits() const {
  int ipgBits = 0;
  auto rv = bcm_port_ifg_get(
      unit_,
      port_,
      static_cast<int>(getSpeed()),
      BCM_PORT_DUPLEX_FULL,
      &ipgBits);
  bcmCheckError(
      rv, "failed to get port inter-packet gap bits for port:", port_);
  return ipgBits;
}

void BcmPort::setInterPacketGapBits(uint32_t ipgBits) {
  uint32_t currentIPG = getInterPacketGapBits();
  if (currentIPG == ipgBits) {
    XLOG(DBG2) << "Port:" << port_
               << " current inter-packet gap bits:" << currentIPG
               << " matches the desired gap bits:" << ipgBits
               << ", skip setInterPacketGapBits";
    return;
  }

  auto rv = bcm_port_ifg_set(
      unit_,
      port_,
      static_cast<int>(getSpeed()),
      BCM_PORT_DUPLEX_FULL,
      ipgBits);
  bcmCheckError(
      rv,
      "failed to set port inter-packet gap bits:",
      ipgBits,
      " for port:",
      port_);
}

} // namespace facebook::fboss
