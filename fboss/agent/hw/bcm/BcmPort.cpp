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
#include <algorithm>
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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/SocUtils.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/init.h>
#include <bcm/link.h>
#include <bcm/port.h>
#include <bcm/qos.h>
#include <bcm/stat.h>
#include <bcm/types.h>
}

using std::shared_ptr;
using std::string;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

using facebook::stats::MonotonicCounter;

namespace {

bool hasPortQueueChanges(
    const shared_ptr<facebook::fboss::Port>& oldPort,
    const shared_ptr<facebook::fboss::Port>& newPort) {
  if (oldPort->getPortQueues().size() != newPort->getPortQueues().size()) {
    return true;
  }

  for (const auto& newQueue : newPort->getPortQueues()) {
    auto oldQueue = oldPort->getPortQueues().at(newQueue->getID());
    if (oldQueue->getName() != newQueue->getName()) {
      return true;
    }
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

bcm_port_phy_fec_t getDesiredFECType(cfg::PortFEC fec) {
  switch (fec) {
    case cfg::PortFEC::ON:
    case cfg::PortFEC::RS_544:
      return bcmPortPhyFecRs544;
      break;
    case cfg::PortFEC::RS_528:
      return bcmPortPhyFecRsFec;
      break;
    case cfg::PortFEC::RS_544_2_N:
      return bcmPortPhyFecRs544_2xN;
      break;
    case cfg::PortFEC::OFF:
      return bcmPortPhyFecNone;
      break;
  }
  throw std::runtime_error("Unsupported fec type in port resource");
}

uint32_t getPhyLaneConfig(
    phy::IpModulation modulation,
    TransmitterTechnology tech) {
  uint32_t laneConfig = 0;

  uint32_t medium = 0;
  switch (tech) {
    case TransmitterTechnology::COPPER:
      medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_COPPER_CABLE;
      break;
    case TransmitterTechnology::BACKPLANE:
      medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_BACKPLANE;
      break;
    case TransmitterTechnology::OPTICAL:
      medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_OPTICS;
      break;
    case TransmitterTechnology::UNKNOWN:
      XLOG(WARNING)
          << "Unknown transmitter technology, fall back to use backplane";
      medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_BACKPLANE;
      break;
  };
  BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_SET(laneConfig, medium);

  switch (modulation) {
    case phy::IpModulation::PAM4:
      // PAM4 + NS
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_PAM4_SET(laneConfig);
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_NS_SET(laneConfig);
      break;
    case phy::IpModulation::NRZ:
      // NRZ
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_NRZ_SET(laneConfig);
      break;
  };

  // always enable DFE
  BCM_PORT_RESOURCE_PHY_LANE_CONFIG_DFE_SET(laneConfig);

  return laneConfig;
}

void updateBcmStat(
    int unit,
    bcm_port_t port,
    std::chrono::seconds now,
    facebook::stats::MonotonicCounter* stat,
    bcm_stat_val_t type,
    int64_t* statVal) {
  // Use the non-sync API to just get the values accumulated in software.
  // The Broadom SDK's counter thread syncs the HW counters to software every
  // 500000us (defined in config.bcm).
  uint64_t value;
  auto ret = bcm_stat_get(unit, port, type, &value);
  bcmCheckError(ret, "Failed to get bcm stat ", type, " for port ", port);

  stat->updateValue(now, value);
  *statVal = value;
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

void BcmPort::reinitPortStats(const std::shared_ptr<Port>& swPort) {
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

  if (swPort) {
    queueManager_->setPortName(portName);
    queueManager_->setupQueueCounters(swPort->getPortQueues());
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

  {
    auto lockedPortStatsPtr = lastPortStats_.wlock();
    *lockedPortStatsPtr =
        BcmPortStats(queueManager_->getNumQueues(cfg::StreamType::UNICAST));
  }
}

BcmPort::BcmPort(BcmSwitch* hw, bcm_port_t port, BcmPlatformPort* platformPort)
    : hw_(hw), port_(port), platformPort_(platformPort), unit_(hw->getUnit()) {
  // Obtain the gport handle from the port handle.
  int rv = bcm_port_gport_get(unit_, port_, &gport_);
  bcmCheckError(rv, "Failed to get gport for BCM port ", port_);

  queueManager_ =
      std::make_unique<BcmPortQueueManager>(hw_, getPortName(), gport_);

  pipe_ = determinePipe();

  XLOG(DBG2) << "created BCM port:" << port_ << ", gport:" << gport_
             << ", FBOSS PortID:" << platformPort_->getPortID();
}

BcmPort::~BcmPort() {
  applyMirrorAction(MirrorAction::STOP, MirrorDirection::INGRESS, sampleDest_);
  applyMirrorAction(MirrorAction::STOP, MirrorDirection::EGRESS, sampleDest_);
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
  }
  initCustomStats();

  // Notify platform port of initial state/speed
  getPlatformPort()->linkSpeedChanged(getSpeed());
  getPlatformPort()->linkStatusChanged(isUp(), isEnabled());
  getPlatformPort()->externalState(PlatformPort::ExternalState::NONE);

  enableLinkscan();
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
  if (!isEnabled()) {
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

bool BcmPort::isEnabled() {
  int enabled;
  auto rv = bcm_port_enable_get(unit_, port_, &enabled);
  bcmCheckError(rv, "Failed to determine if port is already disabled");
  return static_cast<bool>(enabled);
}

bool BcmPort::isUp() {
  if (!isEnabled()) {
    return false;
  }
  int linkStatus;
  auto rv = bcm_port_link_status_get(hw_->getUnit(), port_, &linkStatus);
  bcmCheckError(rv, "could not find if the port ", port_, " is up or down...");
  return linkStatus == BCM_PORT_LINK_STATUS_UP;
}

void BcmPort::enable(const std::shared_ptr<Port>& swPort) {
  if (isEnabled()) {
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
    setPortResource(port);
  } else {
    setSpeed(port);
    // Update FEC settings if needed. Note this is not only
    // on speed change as the port's default speed (say on a
    // cold boot) maybe what is desired by the config. But we
    // may still need to enable FEC
    setFEC(port);
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
  // Update Tx Setting if needed.
  setTxSetting(port);
  setLoopbackMode(port);

  setupStatsIfNeeded(port);

  {
    XLOG(DBG3) << "Saving port settings for " << port->getName();
    auto lockedSettings = programmedSettings_.wlock();
    *lockedSettings = port;
  }
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
        " to ",
        swPort->getIngressVlan());
  }
}

TransmitterTechnology BcmPort::getTransmitterTechnology(
    const std::string& name) {
  // Since we are very unlikely to switch a port from copper to optical
  // while the agent is running, don't make unnecessary attempts to figure
  // out the transmitter technology when we already know what it is.
  if (transmitterTechnology_ != TransmitterTechnology::UNKNOWN) {
    return transmitterTechnology_;
  }
  // 6pack backplane ports will report tech as unknown because this
  // information can't be retrieved via qsfp. These are actually copper,
  // and so should use that instead of any potential default value
  if (name.find("fab") == 0) {
    transmitterTechnology_ = TransmitterTechnology::COPPER;
  } else {
    folly::EventBase evb;
    transmitterTechnology_ =
        getPlatformPort()->getTransmitterTech(&evb).getVia(&evb);
  }
  return transmitterTechnology_;
}

bcm_port_if_t BcmPort::getDesiredInterfaceMode(
    cfg::PortSpeed speed,
    PortID id,
    const std::string& name) {
  TransmitterTechnology transmitterTech = getTransmitterTechnology(name);

  // If speed or transmitter type isn't in map
  try {
    auto result =
        getSpeedToTransmitterTechAndMode().at(speed).at(transmitterTech);
    XLOG(DBG1) << "Getting desired interface mode for port " << id
               << " (speed=" << static_cast<int>(speed)
               << ", tech=" << static_cast<int>(transmitterTech)
               << "). RESULT=" << result;
    return result;
  } catch (const std::out_of_range& ex) {
    throw FbossError(
        "Unsupported speed (",
        speed,
        ") or transmitter technology (",
        transmitterTech,
        ") setting on port ",
        id);
  }
}

cfg::PortSpeed BcmPort::getSpeed() const {
  int curSpeed{0};
  auto rv = bcm_port_speed_get(unit_, port_, &curSpeed);
  bcmCheckError(rv, "Failed to get current speed for port ", port_);
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

void BcmPort::setInterfaceMode(const shared_ptr<Port>& swPort) {
  auto desiredPortSpeed = getDesiredPortSpeed(swPort);
  bcm_port_if_t desiredMode = getDesiredInterfaceMode(
      desiredPortSpeed, swPort->getID(), swPort->getName());

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
    getPlatformPort()->linkSpeedChanged(desiredPortSpeed);
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

  if (!getHW()->getPlatform()->sflowSamplingSupported()) {
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

void BcmPort::setupStatsIfNeeded(const std::shared_ptr<Port>& swPort) {
  if (!shouldReportStats()) {
    return;
  }

  std::shared_ptr<Port> savedPort;
  {
    auto savedSettings = programmedSettings_.rlock();
    savedPort = *savedSettings;
  }

  if (!savedPort || swPort->getName() != savedPort->getName() ||
      hasPortQueueChanges(savedPort, swPort)) {
    reinitPortStats(swPort);
  }
}

PortID BcmPort::getPortID() const {
  return platformPort_->getPortID();
}

std::string BcmPort::getPortName() {
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
  return state->getPorts()->getPortIf(getPortID());
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

void BcmPort::updateStats() {
  // TODO: It would be nicer to use a monotonic clock, but unfortunately
  // the ServiceData code currently expects everyone to use system time.
  if (!shouldReportStats()) {
    return;
  }

  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());

  HwPortStats curPortStats, lastPortStats;
  {
    auto lockedPortStats = lastPortStats_.rlock();
    if (lockedPortStats->has_value()) {
      lastPortStats = curPortStats = (*lockedPortStats)->portStats();
    }
  }

  // All stats start with a unitialized (-1) value. If there are no in discards
  // we will just report that as the monotonic counter. Instead set it to
  // 0 if uninintialized
  curPortStats.inDiscards_ =
      curPortStats.inDiscards_ == hardware_stats_constants::STAT_UNINITIALIZED()
      ? 0
      : curPortStats.inDiscards_;
  updateStat(now, kInBytes(), snmpIfHCInOctets, &curPortStats.inBytes_);
  updateStat(
      now, kInUnicastPkts(), snmpIfHCInUcastPkts, &curPortStats.inUnicastPkts_);
  updateStat(
      now,
      kInMulticastPkts(),
      snmpIfHCInMulticastPkts,
      &curPortStats.inMulticastPkts_);
  updateStat(
      now,
      kInBroadcastPkts(),
      snmpIfHCInBroadcastPkts,
      &curPortStats.inBroadcastPkts_);
  updateStat(
      now, kInDiscardsRaw(), snmpIfInDiscards, &curPortStats.inDiscardsRaw_);
  updateStat(now, kInErrors(), snmpIfInErrors, &curPortStats.inErrors_);
  updateStat(
      now,
      kInIpv4HdrErrors(),
      snmpIpInHdrErrors,
      &curPortStats.inIpv4HdrErrors_);
  updateStat(
      now,
      kInIpv6HdrErrors(),
      snmpIpv6IfStatsInHdrErrors,
      &curPortStats.inIpv6HdrErrors_);
  updateStat(now, kInPause(), snmpDot3InPauseFrames, &curPortStats.inPause_);
  // Egress Stats
  updateStat(now, kOutBytes(), snmpIfHCOutOctets, &curPortStats.outBytes_);
  updateStat(
      now,
      kOutUnicastPkts(),
      snmpIfHCOutUcastPkts,
      &curPortStats.outUnicastPkts_);
  updateStat(
      now,
      kOutMulticastPkts(),
      snmpIfHCOutMulticastPkts,
      &curPortStats.outMulticastPkts_);
  updateStat(
      now,
      kOutBroadcastPkts(),
      snmpIfHCOutBroadcastPckts,
      &curPortStats.outBroadcastPkts_);
  updateStat(
      now, kOutDiscards(), snmpIfOutDiscards, &curPortStats.outDiscards_);
  updateStat(now, kOutErrors(), snmpIfOutErrors, &curPortStats.outErrors_);
  updateStat(now, kOutPause(), snmpDot3OutPauseFrames, &curPortStats.outPause_);

  updateBcmStats(now, &curPortStats);

  setAdditionalStats(now, &curPortStats);

  std::vector<utility::CounterPrevAndCur> toSubtractFromInDiscardsRaw = {
      {lastPortStats.inDstNullDiscards_, curPortStats.inDstNullDiscards_}};
  if (isMmuLossy()) {
    // If MMU setup as lossy, all incoming pause frames will be
    // discarded and will count towards in discards. This makes in discards
    // counter somewhat useless. So instead calculate "in_non_pause_discards",
    // by subtracting the pause frames received from in_discards.
    // TODO: Test if this is true when rx pause is enabled
    toSubtractFromInDiscardsRaw.emplace_back(
        lastPortStats.inPause_, curPortStats.inPause_);
  }
  curPortStats.inDiscards_ += utility::subtractIncrements(
      {lastPortStats.inDiscardsRaw_, curPortStats.inDiscardsRaw_},
      toSubtractFromInDiscardsRaw);

  auto inDiscards = getPortCounterIf(kInDiscards());
  inDiscards->updateValue(now, curPortStats.inDiscards_);

  {
    auto lockedLastPortStatsPtr = lastPortStats_.wlock();
    *lockedLastPortStatsPtr = BcmPortStats(curPortStats, now);
  }

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

void BcmPort::updateStat(
    std::chrono::seconds now,
    folly::StringPiece statKey,
    bcm_stat_val_t type,
    int64_t* statVal) {
  auto stat = getPortCounterIf(statKey);
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

bool BcmPort::isMmuLossy() const {
  return hw_->getMmuState() == BcmSwitch::MmuState::MMU_LOSSY;
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
  auto queueInitStats = folly::copy(portStats_.queueOutDiscardBytes_);
  for (auto cosq = 0; cosq < numUnicastQueues; ++cosq) {
    queueInitStats.emplace(cosq, 0);
  }
  portStats_.set_queueOutDiscardBytes_(queueInitStats);
  portStats_.set_queueOutBytes_(queueInitStats);
  portStats_.set_queueOutPackets_(queueInitStats);
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
  auto bcmStats = lastPortStats_.rlock();
  if (!bcmStats->has_value()) {
    return std::nullopt;
  }
  return (*bcmStats)->portStats();
}

std::chrono::seconds BcmPort::getTimeRetrieved() const {
  auto bcmStats = lastPortStats_.rlock();
  if (!bcmStats->has_value()) {
    return std::chrono::seconds(0);
  }
  return (*bcmStats)->timeRetrieved();
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
  auto* bcmMirror = hw_->getBcmMirrorTable()->getMirrorIf(mirrorName.value());
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

bool BcmPort::shouldReportStats() const {
  return statCollectionEnabled_.load();
}

void BcmPort::destroyAllPortStats() {
  std::map<std::string, stats::MonotonicCounter> swapTo;
  portCounters_.swap(swapTo);

  for (auto& item : swapTo) {
    utility::deleteCounter(item.second.getName());
  }
  queueManager_->destroyQueueCounters();

  {
    auto lockedPortStatsPtr = lastPortStats_.wlock();
    *lockedPortStatsPtr = std::nullopt;
  }
}

void BcmPort::enableStatCollection(const std::shared_ptr<Port>& port) {
  XLOG(DBG2) << "Enabling stats for " << port->getName();

  // Enable packet and byte counter statistic collection.
  auto rv = bcm_port_stat_enable_set(unit_, gport_, true);
  if (rv != BCM_E_EXISTS) {
    // Don't throw an error if counter collection is already enabled
    bcmCheckError(rv, "Unexpected error enabling counter DMA on port ", port_);
  }

  reinitPortStats(port);

  statCollectionEnabled_.store(true);
}

void BcmPort::disableStatCollection() {
  XLOG(DBG2) << "disabling stats for " << getPortName();

  // Disable packet and byte counter statistic collection.
  auto rv = bcm_port_stat_enable_set(unit_, gport_, false);
  bcmCheckError(rv, "Unexpected error disabling counter DMA on port ", port_);

  statCollectionEnabled_.store(false);

  destroyAllPortStats();
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

bool BcmPort::getDesiredFECEnabledStatus(const std::shared_ptr<Port>& swPort) {
  // Ideally FEC should only be enabled for 100G optical ports.
  // But there are NICs which always turn these on regardless
  // of whether they are using optics or not. So to work around
  // that and also to avoid the (very small) latency incurred by
  // FEC where we can, base the decision on both speed and what
  // the platform port tells us

  // Any speed less than 100G should use the OFF option
  auto at_least_100g = swPort->getSpeed() >= cfg::PortSpeed::HUNDREDG;
  return (at_least_100g || swPort->getFEC() != cfg::PortFEC::OFF) &&
      !platformPort_->shouldDisableFEC();
}

uint32_t BcmPort::getCL91FECStatus() const {
  uint32_t cl91Status{0};
  if (SocUtils::isTomahawk(unit_)) {
    auto rv = bcm_port_phy_control_get(
        unit_,
        port_,
        BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91,
        &cl91Status);
    if (rv == BCM_E_UNAVAIL) {
      XLOG(INFO) << "Failed to read if CL91 FEC is enabled: " << bcm_errmsg(rv);
      return 0;
    }
    bcmCheckError(rv, "failed to read if CL91 FEC is enabled");
  }
  return cl91Status;
}

bool BcmPort::isCL91FECApplicable() const {
  return SocUtils::isTomahawk(unit_);
}

void BcmPort::setFEC(const std::shared_ptr<Port>& swPort) {
  // Change FEC setting on an UP port could cause port flap.
  // Only do it when the port is not UP.
  //
  // We are extra conservative here due to bcm case #1134023, where we
  // cannot rely on the value of bcm_port_phy_control_get for
  // BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION on 6.4.10
  if (isUp()) {
    XLOG(DBG1) << "Skip FEC setting on port " << swPort->getID()
               << ", which is UP";
    return;
  }

  auto shouldEnableFec = getDesiredFECEnabledStatus(swPort);
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
  }

  if (SocUtils::isTomahawk(unit_)) {
    // HACK: assume CL91 fec if 100g port, CL74 otherwise.
    //
    // We also make sure cl91 is disabled if we don't want fec to be
    // enabled at all.
    int desiredCl91Status =
        shouldEnableFec && swPort->getSpeed() == cfg::PortSpeed::HUNDREDG;

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
        XLOG(INFO) << "Failed to set CL91 FEC is enabled: " << bcm_errmsg(rv);
        return;
      }
      bcmCheckError(rv, "failed to set cl91 fec setting");
    }
  }
}

void BcmPort::setTxSetting(const std::shared_ptr<Port>& swPort) {
  if (platformPort_->shouldUsePortResourceAPIs()) {
    setTxSettingViaPhyTx(swPort);
  } else {
    setTxSettingViaPhyControl(swPort);
  }
}

// Set the preemphasis (pre-tap; main-tap; post-tap) setting for the transmitter
// and the amplitude control (driver current) values
void BcmPort::setTxSettingViaPhyControl(const std::shared_ptr<Port>& swPort) {
  // Changing the preemphasis/driver-current setting won't cause link flap,
  // so it's safe to not check for port status
  folly::EventBase evb;
  // Get current preemphasis setting
  auto tx = getPlatformPort()->getTxSettings(&evb).getVia(&evb);
  if (!tx.has_value()) {
    // If TxSetting returns no values, it means that we don't need to update
    // tx setting for the port. So do nothing
    return;
  }
  auto correctTx = tx.value();
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
  if (dc != correctTx.driveCurrent && correctTx.driveCurrent != 0) {
    bcm_port_phy_control_set(
        unit_,
        port_,
        BCM_PORT_PHY_CONTROL_DRIVER_CURRENT,
        correctTx.driveCurrent);

    XLOG(DBG1) << "Set drive current on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(correctTx.driveCurrent) << " from "
               << dc;
  }
  if (preTap != correctTx.preTap && correctTx.preTap != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_PRE, correctTx.preTap);
    XLOG(DBG1) << "Set pre-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(correctTx.preTap) << " from " << preTap;
  }
  if (mainTap != correctTx.mainTap && correctTx.mainTap != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_MAIN, correctTx.mainTap);
    XLOG(DBG1) << "Set main-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(correctTx.mainTap) << " from "
               << mainTap;
  }
  if (postTap != correctTx.postTap && correctTx.postTap != 0) {
    bcm_port_phy_control_set(
        unit_, port_, BCM_PORT_PHY_CONTROL_TX_FIR_POST, correctTx.postTap);
    XLOG(DBG1) << "Set post-tap on port " << swPort->getID() << " to be "
               << static_cast<uint32_t>(correctTx.postTap) << " from "
               << postTap;
  }
}

void BcmPort::setTxSettingViaPhyTx(const std::shared_ptr<Port>& swPort) {
  auto profileID = swPort->getProfileID();
  if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
    throw FbossError("Found default profile for port ", swPort->getName());
  }

  auto platformPortEntry = getPlatformPort()->getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError("No PlatformPortEntry found for ", swPort->getName());
  }

  auto platformPortConfig =
      platformPortEntry.value().supportedProfiles.find(profileID);
  if (platformPortConfig == platformPortEntry.value().supportedProfiles.end()) {
    throw FbossError(
        "No speed profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformPortEntry for ",
        swPort->getName());
  }

  auto portProfileConfig = hw_->getPlatform()->getPortProfileConfig(profileID);
  if (!portProfileConfig.has_value()) {
    throw FbossError(
        "No port profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformConfig for ",
        swPort->getName());
  }

  const auto& iphyLaneConfigs =
      utility::getIphyLaneConfigs(platformPortConfig->second.pins.iphy);
  if (iphyLaneConfigs.empty()) {
    XLOG(INFO) << "No iphy lane configs needed to program for "
               << swPort->getName();
    return;
  }

  auto modulation = portProfileConfig->iphy.modulation;
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
        tx.pre2 != txSettings->pre2 || tx.pre != txSettings->pre ||
        tx.main != txSettings->main || tx.post != txSettings->post ||
        tx.post2 != txSettings->post2 || tx.post3 != txSettings->post3) {
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

    tx.pre2 = txSettings->pre2;
    tx.pre = txSettings->pre;
    tx.main = txSettings->main;
    tx.post = txSettings->post;
    tx.post2 = txSettings->post2;
    tx.post3 = txSettings->post3;
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

void BcmPort::setLoopbackMode(const std::shared_ptr<Port>& swPort) {
  int newLoopbackMode{BCM_PORT_LOOPBACK_NONE};
  switch (swPort->getLoopbackMode()) {
    case cfg::PortLoopbackMode::NONE:
      newLoopbackMode = BCM_PORT_LOOPBACK_NONE;
      break;
    case cfg::PortLoopbackMode::PHY:
      newLoopbackMode = BCM_PORT_LOOPBACK_PHY;
      break;
    case cfg::PortLoopbackMode::MAC:
      newLoopbackMode = BCM_PORT_LOOPBACK_MAC;
      break;
  }
  int oldLoopbackMode;
  auto rv = bcm_port_loopback_get(unit_, port_, &oldLoopbackMode);
  bcmCheckError(
      rv, "failed to get loopback mode state for port", swPort->getID());
  if (oldLoopbackMode != newLoopbackMode) {
    rv = bcm_port_loopback_set(unit_, port_, newLoopbackMode);
    bcmCheckError(
        rv,
        "failed to set loopback mode state to ",
        swPort->getLoopbackMode(),
        " for port",
        swPort->getID());
  }
}

bool BcmPort::isFECEnabled() {
  if (platformPort_->shouldUsePortResourceAPIs()) {
    auto curPortResource = getCurrentPortResource(unit_, gport_);
    return curPortResource.fec_type != bcmPortPhyFecNone;
  }

  uint32_t value = BCM_PORT_PHY_CONTROL_FEC_OFF;
  auto rv = bcm_port_phy_control_get(
      unit_, port_, BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION, &value);
  bcmCheckError(rv, "failed to read if FEC is enabled");
  // compare against 'OFF' because result could be 'ON'
  // or 'AUTO'  - not clear how to verify AUTO has gone to yes
  return (
      value != BCM_PORT_PHY_CONTROL_FEC_OFF ||
      (isCL91FECApplicable() && getCL91FECStatus()));
}

void BcmPort::initCustomStats() const {
  int rv = 0;
  if (SocUtils::isTomahawk3(unit_)) {
    rv = bcm_stat_custom_add(
        0, port_, snmpBcmCustomReceive3, bcmDbgCntRxL3DstDiscardDrop);
  } else {
    rv = bcm_stat_custom_add(
        0, port_, snmpBcmCustomReceive3, bcmDbgCntDSTDISCARDDROP);
  }
  bcmCheckError(rv, "Unable to set up custom stat for DST discard drops");
}

void BcmPort::setAdditionalStats(
    std::chrono::seconds now,
    HwPortStats* curPortStats) {
  queueManager_->updateQueueStats(now, curPortStats);
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

void BcmPort::setPause(const std::shared_ptr<Port>& swPort) {
  auto pause = swPort->getPause();
  int expectTx = (pause.tx) ? 1 : 0;
  int expectRx = (pause.rx) ? 1 : 0;

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

uint8_t BcmPort::determinePipe() const {
  // almost certainly open sourced since we updated bcm...
  bcm_info_t info;
  auto rv = bcm_info_get(unit_, &info);
  bcmCheckError(rv, "failed to get unit info");

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
  bcmCheckError(rv, "failed to get dscp map mode");

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

void BcmPort::setPortResource(const std::shared_ptr<Port>& swPort) {
  // Since BRCM SDK 6.5.13, there're a lot of port apis deprecated for PM8x50
  // ports(like TH3). And a new powerful, atomic api bcm_port_resource_get/set
  // is created. Therefore, we're able to configure different attributes for a
  // bcm_port_resource, e.g. speed, fec_type and etc, all together.
  auto curPortResource = getCurrentPortResource(unit_, gport_);
  auto desiredPortResource = curPortResource;

  TransmitterTechnology tech = getTransmitterTechnology(swPort->getName());
  XLOG(DBG1) << swPort->getName() << " has medium type: "
             << apache::thrift::util::enumNameSafe(tech);
  if (swPort->getProfileID() != cfg::PortProfileID::PROFILE_DEFAULT) {
    auto profileID = swPort->getProfileID();
    const auto profileConf =
        hw_->getPlatform()->getPortProfileConfig(profileID);
    if (!profileConf) {
      throw FbossError(
          "Platform doesn't support speed profile: ",
          apache::thrift::util::enumNameSafe(profileID));
    }

    XLOG(DBG1) << "Program port resource based on speed profile: "
               << apache::thrift::util::enumNameSafe(profileID);
    desiredPortResource.speed = static_cast<int>((*profileConf).get_speed());
    desiredPortResource.fec_type =
        utility::phyFecModeToBcmPortPhyFec((*profileConf).get_iphy().get_fec());
    desiredPortResource.phy_lane_config =
        getPhyLaneConfig((*profileConf).get_iphy().get_modulation(), tech);
  } else {
    // in case we don't have the new config yet
    cfg::PortSpeed desiredPortSpeed = getDesiredPortSpeed(swPort);
    desiredPortResource.speed = static_cast<int>(getDesiredPortSpeed(swPort));
    bool enableFec = getDesiredFECEnabledStatus(swPort);
    cfg::PortFEC desiredFEC = enableFec ? swPort->getFEC() : cfg::PortFEC::OFF;
    desiredPortResource.fec_type = getDesiredFECType(desiredFEC);
    desiredPortResource.phy_lane_config =
        getDesiredPhyLaneConfig(tech, desiredPortSpeed);
  }

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
    return;
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

  if (curPortResource.speed != desiredPortResource.speed) {
    getPlatformPort()->linkSpeedChanged(
        cfg::PortSpeed(desiredPortResource.speed));
  }
}

void BcmPort::updateBcmStats(
    std::chrono::seconds now,
    HwPortStats* curPortStats) {
  // Stat types supported on 6.5.13 or later only
  if (hw_->getPlatform()->isCosSupported()) {
    // Stats not supported bny TD2
    updateBcmStat(
        unit_,
        port_,
        now,
        getPortCounterIf(kOutEcnCounter()),
        snmpBcmTxEcnErrors,
        &curPortStats->outEcnCounter_);
  }
  updateBcmStat(
      unit_,
      port_,
      now,
      getPortCounterIf(kInDstNullDiscards()),
      snmpBcmCustomReceive3,
      &curPortStats->inDstNullDiscards_);
}

} // namespace facebook::fboss
