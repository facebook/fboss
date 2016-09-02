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
  outQueueLen_ = statMap->getLockableStat(statName("out_queue_length"),
                                          &expType);
  auto histMap = fbData->getHistogramMap();
  stats::ExportedHistogram pktLenHist(1, 0, kInPktLengthStats.size());
  inPktLengths_ = histMap->getOrCreateLockableHistogram(
      statName("in_pkt_lengths"), &pktLenHist);
  outPktLengths_ = histMap->getOrCreateLockableHistogram(
      statName("out_pkt_lengths"), &pktLenHist);

  setConfiguredMaxSpeed();

  VLOG(2) << "created BCM port:" << port_ << ", gport:" << gport_
          << ", FBOSS PortID:" << platformPort_->getPortID();
}

void BcmPort::init(bool warmBoot) {
  bool up = false;
  if (warmBoot) {
    // Get port status from HW on warm boot.
    // All ports are initially down on a cold boot.
    int linkStatus;
    opennsl_port_link_status_get(unit_, port_, &linkStatus);
    up = (linkStatus == OPENNSL_PORT_LINK_STATUS_UP);
  } else {
    // In open source code, we don't have any guarantees for the
    // state of the port at startup. Bringing them down guarantees
    // that things are in a known state.
    //
    // We should only be doing this on cold boot, since warm booting
    // should be initializing the state for us.
    auto rv = opennsl_port_enable_set(unit_, port_, false);
    bcmCheckError(rv, "failed to set port to known state: ", port_);
  }

  setPortStatus(up);

  // Linkscan should be enabled if the port is enabled already
  auto linkscan = isEnabled() ? OPENNSL_LINKSCAN_MODE_SW :
    OPENNSL_LINKSCAN_MODE_NONE;
  auto rv = opennsl_linkscan_mode_set(unit_, port_, linkscan);
  bcmCheckError(rv, "failed to set initial linkscan mode on port ", port_);
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
  if (!isEnabled()) {
    // Already disabled
    return;
  }

  auto pbmp = getPbmp();
  for (auto entry : swPort->getVlans()) {
    auto rv = opennsl_vlan_port_remove(unit_, entry.first, pbmp);
    bcmCheckError(rv, "failed to remove disabled port ",
                  swPort->getID(), " from VLAN ", entry.first);
  }

  // Disable packet and byte counter statistic collection.
  auto rv = opennsl_port_stat_enable_set(unit_, gport_, false);
  bcmCheckError(rv, "Unexpected error disabling counter DMA on port ",
                swPort->getID());

  // Disable linkscan
  rv = opennsl_linkscan_mode_set(unit_, port_, OPENNSL_LINKSCAN_MODE_NONE);
  bcmCheckError(rv, "Failed to disable linkscan on port ", swPort->getID());

  rv = opennsl_port_enable_set(unit_, port_, false);
  bcmCheckError(rv, "failed to disable port ", swPort->getID());
}

bool BcmPort::isEnabled() {
  int enabled;
  auto rv = opennsl_port_enable_get(unit_, port_, &enabled);
  bcmCheckError(rv, "Failed to determine if port is already disabled");
  return static_cast<bool>(enabled);
}

void BcmPort::enable(const std::shared_ptr<Port>& swPort) {
  if (isEnabled()) {
    // Port is already enabled, don't need to do anything
    return;
  }

  auto pbmp = getPbmp();
  opennsl_pbmp_t emptyPortList;
  OPENNSL_PBMP_CLEAR(emptyPortList);
  int rv;
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
  setFEC(port);
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

opennsl_port_if_t BcmPort::getDesiredInterfaceMode(cfg::PortSpeed speed,
                                                   PortID id,
                                                   std::string name) {
  static constexpr const char* sixPackLCFabricPortPrefix = "fab";
  switch (speed) {
  case cfg::PortSpeed::HUNDREDG:
    return OPENNSL_PORT_IF_CR4;
  case cfg::PortSpeed::FIFTYG:
    // TODO(aeckert): CR2 does not exist in opennsl 6.3.6.
    // Remove this ifdef once fully on 6.4.6
#if defined(OPENNSL_PORT_IF_CR2)
    return OPENNSL_PORT_IF_CR2;
#else
    throw FbossError("Unsupported speed (", speed,
                     ") setting on port ", id);
#endif
  case cfg::PortSpeed::FORTYG:
    // TODO: Currently, we are finding if the port is backplane port or not by
    // its name. A better way is to include this information in the config:
    // t9112164.
    if (name.find(sixPackLCFabricPortPrefix) == 0) {
      return OPENNSL_PORT_IF_CR4;
    } else {
      return OPENNSL_PORT_IF_SR4;
    }
  case cfg::PortSpeed::TWENTYFIVEG:
    return OPENNSL_PORT_IF_CR;
  case cfg::PortSpeed::TWENTYG:
  case cfg::PortSpeed::XG:
    return OPENNSL_PORT_IF_SFI;
  case cfg::PortSpeed::GIGE:
    return OPENNSL_PORT_IF_GMII;
  default:
    throw FbossError("Unsupported speed (", speed,
                     ") setting on port ", id);
  }
}

void BcmPort::setSpeed(const shared_ptr<Port>& swPort) {
  if (isEnabled()) {
    LOG(ERROR) << "Cannot set port speed while the port is enabled. Port: "
               << swPort->getName() << " id: " << swPort->getID();
    return;
  }
  int ret;
  cfg::PortSpeed desiredPortSpeed;
  if (swPort->getSpeed() == cfg::PortSpeed::DEFAULT) {
    int speed;
    ret = opennsl_port_speed_max(unit_, port_, &speed);
    bcmCheckError(ret, "failed to get max speed for port", swPort->getID());
    desiredPortSpeed = cfg::PortSpeed(speed);
  } else {
    desiredPortSpeed = swPort->getSpeed();
  }

  opennsl_port_if_t desiredMode = getDesiredInterfaceMode(desiredPortSpeed,
                                                          swPort->getID(),
                                                          swPort->getName());

  opennsl_port_if_t curMode;
  ret = opennsl_port_interface_get(unit_, port_, &curMode);
  bcmCheckError(ret,
                "Failed to get current interface setting for port ",
                swPort->getID());

  bool updateSpeed = false;

  if (curMode != desiredMode) {
    ret = opennsl_port_interface_set(unit_, port_, desiredMode);
    bcmCheckError(
        ret, "failed to set interface type for port ", swPort->getID());
    // Changes to the interface setting only seem to take effect on the next
    // call to opennsl_port_speed_set().  Therefore make sure we update the
    // speed below, even if the speed is already at the desired setting.
    updateSpeed = true;
  }

  int desiredSpeed = static_cast<int>(desiredPortSpeed);
  // Unnecessarily updating BCM port speed actually causes
  // the port to flap, even if this should be a noop, so check current
  // speed before making speed related changes. Doing so fixes
  // the interface flaps we were seeing during warm boots

  int curSpeed{0};
  ret = opennsl_port_speed_get(unit_, port_, &curSpeed);
  bcmCheckError(
    ret, "Failed to get current speed for port ", swPort->getID());

  if (!updateSpeed && desiredMode != OPENNSL_PORT_IF_KR4) {
    updateSpeed |= (curSpeed != desiredSpeed);
  }

  if (updateSpeed) {
    if (desiredMode == OPENNSL_PORT_IF_KR4) {
      // We don't need to set speed when mode is KR4, since ports in KR4 mode
      // do autonegotiation to figure out the speed.
      setKR4Ability();
    } else {
      ret = opennsl_port_speed_set(unit_, port_, desiredSpeed);
      bcmCheckError(ret,
                    "failed to set speed to ",
                    desiredSpeed, " from ", curSpeed,
                    ", on port ",
                    swPort->getID());
    }
    getPlatformPort()->linkSpeedChanged(desiredPortSpeed);
  }
}

PortID BcmPort::getPortID() const {
  return platformPort_->getPortID();
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
  return state->getPorts()->getPortIf(getPortID());
}

void BcmPort::setPortStatus(bool up) {
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
    outQueueLen_.addValue(now.count(), qlength);
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
    stats::ExportedHistogramMapImpl::LockableHistogram* hist,
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
  auto guard = hist->makeLockGuard();
  for (int idx = 0; idx < stats.size(); ++idx) {
    hist->addValueLocked(guard, now.count(), idx, counters[idx]);
  }
}

cfg::PortState BcmPort::getState() {
  int rv;
  int enabled;
  int linkStatus;
  rv = opennsl_port_enable_get(hw_->getUnit(), port_, &enabled);
  bcmCheckError(rv, "could not find if port ", port_, " is enabled or not...");
  if (!enabled) {
    return cfg::PortState::POWER_DOWN;
  }
  rv = opennsl_port_link_status_get(hw_->getUnit(), port_, &linkStatus);
  bcmCheckError(rv, "could not find if the port ", port_, " is up or down...");
  if (linkStatus == OPENNSL_PORT_LINK_STATUS_UP) {
    return cfg::PortState::UP;
  } else {
    return cfg::PortState::DOWN;
  }
}

}} // namespace facebook::fboss
