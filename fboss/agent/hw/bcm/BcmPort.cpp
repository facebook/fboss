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

#include <cstdlib>
#include <cstring>

#include "common/stats/MonotonicCounter.h"
#include "common/stats/ServiceData.h"
#include <folly/Conv.h>

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <opennsl/port.h>
#include <opennsl/stat.h>
}

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::string;

namespace facebook { namespace fboss {

static const std::vector<opennsl_stat_val_t> kInPktLengthStats = {
  snmpOpenNSLReceivedPkts64Octets,
  snmpOpenNSLReceivedPkts65to127Octets,
  snmpOpenNSLReceivedPkts128to255Octets,
  snmpOpenNSLReceivedPkts256to511Octets,
  snmpOpenNSLReceivedPkts512to1023Octets,
  snmpOpenNSLReceivedPkts1024to1518Octets,
  snmpOpenNSLReceivedPkts1519to2047Octets,
  snmpOpenNSLReceivedPkts2048to4095Octets,
  snmpOpenNSLReceivedPkts4095to9216Octets,
  snmpOpenNSLReceivedPkts9217to16383Octets,
};
static const std::vector<opennsl_stat_val_t> kOutPktLengthStats = {
  snmpOpenNSLTransmittedPkts64Octets,
  snmpOpenNSLTransmittedPkts65to127Octets,
  snmpOpenNSLTransmittedPkts128to255Octets,
  snmpOpenNSLTransmittedPkts256to511Octets,
  snmpOpenNSLTransmittedPkts512to1023Octets,
  snmpOpenNSLTransmittedPkts1024to1518Octets,
  snmpOpenNSLTransmittedPkts1519to2047Octets,
  snmpOpenNSLTransmittedPkts2048to4095Octets,
  snmpOpenNSLTransmittedPkts4095to9216Octets,
  snmpOpenNSLTransmittedPkts9217to16383Octets,
};

BcmPort::BcmPort(BcmSwitch* hw, opennsl_port_t port,
                 BcmPlatformPort* platformPort)
    : hw_(hw),
      port_(port),
      platformPort_(platformPort) {
  // Obtain the gport handle from the port handle.
  int unit = hw_->getUnit();
  int rv = opennsl_port_gport_get(unit, port_, &gport_);
  bcmCheckError(rv, "Failed to get gport for BCM port ", port_);

  disablePause();

  // Initialize our stats data structures
  auto statMap = fbData->getStatMap();
  const auto expType = stats::AVG;
  outQueueLen_ = statMap->getLockAndStatItem(statName("out_queue_length"),
                                             &expType);
  auto histMap = fbData->getHistogramMap();
  stats::ExportedHistogram pktLenHist(1, 0, kInPktLengthStats.size());
  inPktLengths_ = histMap->getOrCreateUnlocked(statName("in_pkt_lengths"),
                                               &pktLenHist);
  outPktLengths_ = histMap->getOrCreateUnlocked(statName("out_pkt_lengths"),
                                                &pktLenHist);

  VLOG(2) << "created BCM port:" << port_ << ", gport:" << gport_
          << ", FBOSS PortID:" << platformPort_->getPortID();
}

void BcmPort::init(bool warmBoot) {
  int unit = hw_->getUnit();

  if (warmBoot) {
    // Get port status from HW
    int linkStatus;
    opennsl_port_link_status_get(unit, port_, &linkStatus);
    setPortStatus(linkStatus);
  } else {
    // Enable packet and byte counter statistic collection.
    int enable = 1;
    int rv = opennsl_port_stat_enable_set(unit, gport_, enable);
    // We will get error messages if BcmSwitch::init() is called after it is
    // initialized. In other words, BcmSwitch::init(), or
    // opennsl_port_stat_enable_set(), is not idempotent. We omit the
    // OPENNSL_E_EXISTS error in particular.
    if (rv != OPENNSL_E_EXISTS) {
        bcmCheckError(rv);
    }
    // All ports are initially down on a cold boot.
    setPortStatus(false);
  }
}

void BcmPort::setPortStatus(int linkstatus) {
  bool up = (linkstatus == OPENNSL_PORT_LINK_STATUS_UP);

  int enabled = 1;
  int rv = opennsl_port_enable_get(hw_->getUnit(), port_, &enabled);
  // We ignore the return value.  If we fail to get the port status
  // we just tell the platformPort_ that it is enabled.

  platformPort_->linkStatusChanged(up, enabled);
}

std::string BcmPort::statName(folly::StringPiece name) const {
  return folly::to<string>("port", platformPort_->getPortID(), ".", name);
}

void BcmPort::updateStats() {
  // TODO: It would be nicer to use a monotonic clock, but unfortunately
  // the ServiceData code currently expects everyone to use system time.
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());

  updateStat(now, &inBytes_, opennsl_spl_snmpIfHCInOctets);
  updateStat(now, &inUnicastPkts_, opennsl_spl_snmpIfHCInUcastPkts);
  updateStat(now, &inMulticastPkts_, opennsl_spl_snmpIfHCInMulticastPkts);
  updateStat(now, &inBroadcastPkts_, opennsl_spl_snmpIfHCInBroadcastPkts);
  updateStat(now, &inDiscards_, opennsl_spl_snmpIfInDiscards);
  updateStat(now, &inErrors_, opennsl_spl_snmpIfInErrors);

  updateStat(now, &outBytes_, opennsl_spl_snmpIfHCOutOctets);
  updateStat(now, &outUnicastPkts_, opennsl_spl_snmpIfHCOutUcastPkts);
  updateStat(now, &outMulticastPkts_, opennsl_spl_snmpIfHCOutMulticastPkts);
  updateStat(now, &outBroadcastPkts_, opennsl_spl_snmpIfHCOutBroadcastPckts);
  updateStat(now, &outDiscards_, opennsl_spl_snmpIfOutDiscards);
  updateStat(now, &outErrors_, opennsl_spl_snmpIfOutErrors);

  setAdditionalStats(now);

  // Update the queue length stat
  uint32_t qlength;
  auto ret = opennsl_port_queued_count_get(hw_->getUnit(), port_, &qlength);
  if (OPENNSL_FAILURE(ret)) {
    LOG(ERROR) << "Failed to get queue length for port " << port_
               << " :" << opennsl_errmsg(ret);
  } else {
    SpinLockHolder guard(outQueueLen_.first.get());
    outQueueLen_.second->addValue(now, qlength);
    // TODO: outQueueLen_ only exports the average queue length over the last
    // 60 seconds, 10 minutes, etc.
    // We should also export the current value.  We could use a simple counter
    // or a dynamic counter for this.
  }

  // Update the packet length histograms
  updatePktLenHist(now, &inPktLengths_, kInPktLengthStats);
  updatePktLenHist(now, &outPktLengths_, kOutPktLengthStats);
};

void BcmPort::updateStat(std::chrono::seconds now,
                         stats::MonotonicCounter* stat,
                         opennsl_stat_val_t type) {
  // Use the non-sync API to just get the values accumulated in software.
  // The Broadom SDK's counter thread syncs the HW counters to software every
  // 500000us (defined in config.bcm).
  uint64_t value;
  auto ret = opennsl_stat_get(hw_->getUnit(), port_, type, &value);
  if (OPENNSL_FAILURE(ret)) {
    LOG(ERROR) << "Failed to get stat " << type << " for port " << port_
               << " :" << opennsl_errmsg(ret);
    return;
  }
  stat->updateValue(now, value);
}

void BcmPort::updatePktLenHist(
    std::chrono::seconds now,
    stats::ExportedHistogramMap::LockAndHistogram* hist,
    const std::vector<opennsl_stat_val_t>& stats) {
  // Get the counter values
  uint64_t counters[10];
  // opennsl_stat_multi_get() unfortunately doesn't correctly const qualify
  // it's stats arguments right now.
  opennsl_stat_val_t* statsArg =
      const_cast<opennsl_stat_val_t*>(&stats.front());
  auto ret = opennsl_stat_multi_get(hw_->getUnit(), port_,
                                stats.size(), statsArg, counters);
  if (OPENNSL_FAILURE(ret)) {
    LOG(ERROR) << "Failed to get packet length stats for port " << port_
               << " :" << opennsl_errmsg(ret);
    return;
  }

  // Update the histogram
  SpinLockHolder guard(hist->first.get());
  for (int idx = 0; idx < stats.size(); ++idx) {
    hist->second->addValue(now, idx, counters[idx]);
  }
}

}} // namespace facebook::fboss
