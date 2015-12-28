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
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"

extern "C" {
#include <opennsl/link.h>
#include <opennsl/port.h>
#include <opennsl/stat.h>
}

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::string;
using std::shared_ptr;

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
      platformPort_(platformPort),
      unit_(hw->getUnit()) {
  // Obtain the gport handle from the port handle.
  int rv = opennsl_port_gport_get(unit_, port_, &gport_);
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

  setConfiguredMaxSpeed();

  VLOG(2) << "created BCM port:" << port_ << ", gport:" << gport_
          << ", FBOSS PortID:" << platformPort_->getPortID();
}

void BcmPort::init(bool warmBoot) {
  if (warmBoot) {
    // Get port status from HW
    int linkStatus;
    opennsl_port_link_status_get(unit_, port_, &linkStatus);
    setPortStatus(linkStatus);
  } else {
    // All ports are initially down on a cold boot.
    setPortStatus(false);
  }
}

bool BcmPort::supportsSpeed(cfg::PortSpeed speed) {
  // It would be nice if we could use the port_ability api here, but
  // that struct changes based on how many lanes are active. So does
  // opennsl_port_speed_max.
  //
  // Instead, we store the speed set in the bcm config file. This will
  // not work correctly if we performed a warm boot and the config
  // file changed port speeds. However, this is not supported by
  // broadcom for warm boot so this approach should be alright.
  return speed <= configuredMaxSpeed_;
}

opennsl_pbmp_t BcmPort::getPbmp() {
  opennsl_pbmp_t pbmp;
  OPENNSL_PBMP_PORT_SET(pbmp, port_);
  return pbmp;
}

void BcmPort::disable(const std::shared_ptr<Port>& swPort) {
  int enabled;
  auto rv = opennsl_port_enable_get(unit_, port_, &enabled);
  bcmCheckError(rv, "Failed to determine if port is already disabled");
  if (!enabled) {
    // Already disabled
    return;
  }

  auto pbmp = getPbmp();
  for (auto entry : swPort->getVlans()) {
    rv = opennsl_vlan_port_remove(unit_, entry.first, pbmp);
    bcmCheckError(rv, "failed to remove disabled port ",
                  swPort->getID(), " from VLAN ", entry.first);
  }

  // Disable packet and byte counter statistic collection.
  rv = opennsl_port_stat_enable_set(unit_, gport_, false);
  bcmCheckError(rv, "Unexpected error disabling counter DMA on port ",
                swPort->getID());

  // Disable linkscan
  rv = opennsl_linkscan_mode_set(unit_, port_, OPENNSL_LINKSCAN_MODE_NONE);
  bcmCheckError(rv, "Failed to disable linkscan on port ", swPort->getID());

  rv = opennsl_port_enable_set(unit_, port_, false);
  bcmCheckError(rv, "failed to disable port ", swPort->getID());
}


void BcmPort::enable(const std::shared_ptr<Port>& swPort) {
  int enabled;
  auto rv = opennsl_port_enable_get(unit_, port_, &enabled);
  bcmCheckError(rv, "Failed to determine if port is already disabled");
  if (enabled) {
    // Port is already enabled, don't need to do anything
    return;
  }

  auto pbmp = getPbmp();
  opennsl_pbmp_t emptyPortList;
  OPENNSL_PBMP_CLEAR(emptyPortList);
  for (auto entry : swPort->getVlans()) {
    if (!entry.second.tagged) {
      rv = opennsl_vlan_port_add(unit_, entry.first, pbmp, pbmp);
    } else {
      rv = opennsl_vlan_port_add(unit_, entry.first, pbmp, emptyPortList);
    }
    bcmCheckError(rv, "failed to add enabled port ",
                  swPort->getID(), " to VLAN ", entry.first);
  }

  // Drop packets to/from this port that are tagged with a VLAN that this
  // port isn't a member of.
  rv = opennsl_port_vlan_member_set(unit_, port_,
                                    OPENNSL_PORT_VLAN_MEMBER_INGRESS |
                                    OPENNSL_PORT_VLAN_MEMBER_EGRESS);
  bcmCheckError(rv, "failed to set VLAN filtering on port ",
                swPort->getID());

  // Set the speed and ingress vlan before enabling
  program(swPort);

  // Enable packet and byte counter statistic collection.
  rv = opennsl_port_stat_enable_set(unit_, gport_, true);
  if (rv != OPENNSL_E_EXISTS) {
    // Don't throw an error if counter collection is already enabled
    bcmCheckError(rv, "Unexpected error enabling counter DMA on port ",
                  swPort->getID());
  }

  // Enable linkscan
  rv = opennsl_linkscan_mode_set(unit_, port_, OPENNSL_LINKSCAN_MODE_SW);
  bcmCheckError(rv, "Failed to enable linkscan on port ", swPort->getID());

  rv = opennsl_port_enable_set(unit_, port_, true);
  bcmCheckError(rv, "failed to enable port ", swPort->getID());
}


void BcmPort::program(const shared_ptr<Port>& port) {
  setIngressVlan(port);
  setSpeed(port);
}

void BcmPort::setIngressVlan(const shared_ptr<Port>& swPort) {
  opennsl_vlan_t currVlan;
  auto rv = opennsl_port_untagged_vlan_get(unit_, port_, &currVlan);
  bcmCheckError(rv, "failed to get ingress VLAN for port ", swPort->getID());

  opennsl_vlan_t bcmVlan = swPort->getIngressVlan();
  if (bcmVlan != currVlan) {
    rv = opennsl_port_untagged_vlan_set(unit_, port_, bcmVlan);
    bcmCheckError(rv, "failed to set ingress VLAN for port ",
                  swPort->getID(), " to ", swPort->getIngressVlan());
  }
}

void BcmPort::setSpeed(const shared_ptr<Port>& swPort) {
  int speed;
  int ret;
  switch (swPort->getSpeed()) {
  case cfg::PortSpeed::DEFAULT:
    ret = opennsl_port_speed_max(unit_, port_, &speed);
    bcmCheckError(ret, "failed to get max speed for port", swPort->getID());
    break;
  case cfg::PortSpeed::HUNDREDG:
  case cfg::PortSpeed::FIFTYG:
  case cfg::PortSpeed::FORTYG:
  case cfg::PortSpeed::TWENTYFIVEG:
  case cfg::PortSpeed::TWENTYG:
  case cfg::PortSpeed::XG:
  case cfg::PortSpeed::GIGE:
    speed = static_cast<int>(swPort->getSpeed());
    break;
  default:
    throw FbossError("Unsupported speed (", swPort->getSpeed(),
                     ") setting on port ", swPort->getID());
    break;
  }
  // We always want to put 10G ports in SFI mode.
  // Note that this call must be made before opennsl_port_speed_set().
  bool updateSpeed = false;
  if (speed == 10000) {
    opennsl_port_if_t curMode;
    ret = opennsl_port_interface_get(unit_, port_, &curMode);
    if (curMode != OPENNSL_PORT_IF_SFI) {
      // Changes to the interface setting only seem to take effect on the next
      // call to opennsl_port_speed_set().  Therefore make sure we update the
      // speed below, even if the speed is already at the desired setting.
      updateSpeed = true;
      ret = opennsl_port_interface_set(unit_, port_, OPENNSL_PORT_IF_SFI);
      bcmCheckError(ret, "failed to set interface type for port ",
                    swPort->getID());
    }
  }

  if (speed == 40000) {
    opennsl_port_if_t curMode;
    ret = opennsl_port_interface_get(unit_, port_, &curMode);
    if (curMode != OPENNSL_PORT_IF_XLAUI) {
      ret = opennsl_port_interface_set(unit_, port_, OPENNSL_PORT_IF_XLAUI);
      bcmCheckError(ret, "failed to set interface type for port ",
                    swPort->getID());
    }
  }

  if (!updateSpeed) {
    // Unnecessarily updating BCM port speed actually causes
    // the port to flap, even if this should be a noop, so check current
    // speed before making speed related changes. Doing so fixes
    // the interface flaps we were seeing during warm boots
    int curSpeed;
    ret = opennsl_port_speed_get(unit_, port_, &curSpeed);
    bcmCheckError(ret, "Failed to get current speed for port",
                  swPort->getID());
    updateSpeed = (curSpeed != speed);
  }
  if (updateSpeed) {
    ret = opennsl_port_speed_set(unit_, port_, speed);
    bcmCheckError(ret, "failed to set speed, ", speed,
                  ", to port ", swPort->getID());
  }

}

PortID BcmPort::getPortID() const {
  return platformPort_->getPortID();
}

cfg::PortSpeed BcmPort::maxLaneSpeed() const {
  return platformPort_->maxLaneSpeed();
}

std::shared_ptr<Port> BcmPort::getSwitchStatePort(
    const std::shared_ptr<SwitchState>& state) const {
  return state->getPort(getPortID());
}

std::shared_ptr<Port> BcmPort::getSwitchStatePortIf(
    const std::shared_ptr<SwitchState>& state) const {
  return state->getPorts()->getPortIf(getPortID());
}

void BcmPort::setPortStatus(int linkstatus) {
  bool up = (linkstatus == OPENNSL_PORT_LINK_STATUS_UP);

  int enabled = 1;
  int rv = opennsl_port_enable_get(unit_, port_, &enabled);
  // We ignore the return value.  If we fail to get the port status
  // we just tell the platformPort_ that it is enabled.

  platformPort_->linkStatusChanged(up, enabled);
}

void BcmPort::registerInPortGroup(BcmPortGroup* portGroup) {
  portGroup_ = portGroup;
  VLOG(2) << "Port " << getPortID() << " registered in PortGroup with "
          << "controlling port " << portGroup->controllingPort()->getPortID();
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
  auto ret = opennsl_port_queued_count_get(unit_, port_, &qlength);
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
  auto ret = opennsl_stat_get(unit_, port_, type, &value);
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
  auto ret = opennsl_stat_multi_get(unit_, port_,
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
