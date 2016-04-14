/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include <boost/foreach.hpp>
#include <folly/IPAddress.h>
#include <folly/io/Cursor.h>  // TODO: Remove this when AddHostsForDemo are removed

#include "SaiSwitch.h"
#include "SaiVrfTable.h"
#include "SaiNextHopTable.h"
#include "SaiHostTable.h"
#include "SaiHost.h"
#include "SaiRxPacket.h"
#include "SaiTxPacket.h"
#include "SaiPortTable.h"
#include "SaiPortBase.h"
#include "SaiIntfTable.h"
#include "SaiRouteTable.h"
#include "SaiWarmBootCache.h"
#include "SaiError.h"
#include "SaiIntf.h"

#include "fboss/agent/packet/ArpHdr.h" // TODO: Remove this when AddHostsForDemo are removed
#include "fboss/agent/ArpHandler.h"    // TODO: Remove this when AddHostsForDemo are removed
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/VlanMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteDelta.h"

#include "fboss/agent/SwitchStats.h"

using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::string;

using folly::make_unique;
using folly::IPAddress;

using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachRemoved;


//extern "C" {
//// Forward declaration for sai_service_method_table_initialize function which is not
//// currently added to some SAI header like saiplatform.h(will propagate this to SAI community) 
//// so has to be defined here. A customer's SAI adapter must provide its implementation.
//sai_status_t sai_service_method_table_initialize(service_method_table_t* services);
//}

namespace facebook { namespace fboss {

// TODO: remove this when SAI callbacks support pointer to user data storage.
SaiSwitch* SaiSwitch::instance_ = nullptr;

facebook::fboss::cfg::PortSpeed GetPortSpeed(int port) {
  VLOG(4) << "Entering " << __FUNCTION__;

  return facebook::fboss::cfg::PortSpeed::XG;
}

SaiSwitch::SaiSwitch(SaiPlatformBase *platform)
  : platform_(platform)
  , portTable_(new SaiPortTable(this))
  , intfTable_(new SaiIntfTable(this))
  , hostTable_(new SaiHostTable(this))
  , routeTable_(new SaiRouteTable(this))
  , vrfTable_(new SaiVrfTable(this))
  , nextHopTable_(new SaiNextHopTable(this))
  , warmBootCache_(new SaiWarmBootCache(this))
  , lockFd(-1) {
  VLOG(4) << "Entering " << __FUNCTION__;

  //sai_status_t status = sai_service_method_table_initialize(&service_);
  //if (status != SAI_STATUS_SUCCESS) {
  //  throw SaiFatal("Failed to initialize service method table. Error: ", status);
  //}

  sai_api_initialize(0, &service_);

  sai_api_query(SAI_API_SWITCH, (void **) &saiSwitchApi_);
  sai_api_query(SAI_API_VLAN, (void **) &saiVlanApi_);
  //sai_api_query(SAI_API_PORT, (void **) &saiPortApi_);
  sai_api_query(SAI_API_ROUTER_INTERFACE, (void **) &saiRouterIntfApi_);
  sai_api_query(SAI_API_ROUTE, (void **) &saiRouteApi_);
  //sai_api_query(SAI_API_ACL, (void **) &saiAclApi_);
  sai_api_query(SAI_API_VIRTUAL_ROUTER, (void **) &saiVrfApi_);
  sai_api_query(SAI_API_NEIGHBOR, (void **) &saiNeighborApi_);
  sai_api_query(SAI_API_NEXT_HOP, (void **) &saiNextHopApi_);
  sai_api_query(SAI_API_NEXT_HOP_GROUP, (void **) &saiNextHopGroupApi_);
  sai_api_query(SAI_API_HOST_INTERFACE, (void **) &saiHostIntfApi_);

  // TODO: remove this when SAI callbacks support pointer to user data storage.
  SaiSwitch::instance_ = this;
}

SaiSwitch::~SaiSwitch() {
  VLOG(4) << "Entering " << __FUNCTION__;

  saiNextHopApi_ = nullptr;
  saiAclApi_ = nullptr;
  saiHostInterfaceApi_ = nullptr;
  saiNeighborApi_  = nullptr;
  saiRouterIntfApi_ = nullptr;
  saiRouteApi_ = nullptr;
  saiVrfApi_ = nullptr;
  saiPortApi_ = nullptr;
  saiVlanApi_ = nullptr;
  saiSwitchApi_ = nullptr;
  saiHostIntfApi_ = nullptr;

  sai_api_uninitialize();

  service_ = {nullptr, nullptr};
  
  ReleaseLock();
}

std::shared_ptr<SwitchState> SaiSwitch::GetColdBootSwitchState() const {
  auto bootState = make_shared<SwitchState>();
  
  // On cold boot all ports are in Vlan 1
  auto vlanMap = make_shared<VlanMap>();
  auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
  Vlan::MemberPorts memberPorts;

  // Cold boot - put all ports in Vlan 1
  for(uint32_t nPort = 0; nPort < saiPortList_.count; ++nPort) {
    sai_object_id_t saiPort = saiPortList_.list[nPort];
    PortID portID = portTable_->GetPortId(saiPort);

    string name = folly::to<string>("port", portID);
    bootState->registerPort(portID, name);
    memberPorts.insert(std::make_pair(PortID(saiPort), false));
  }

  vlan->setPorts(memberPorts);
  bootState->addVlan(vlan);
  return bootState;
}

std::shared_ptr<SwitchState> SaiSwitch::GetWarmBootSwitchState() const {
  auto warmBootState = GetColdBootSwitchState();
  for (auto port : *warmBootState->getPorts()) {
    int portEnabled;
    
    port->setState(portEnabled == 1 ? cfg::PortState::UP: cfg::PortState::DOWN);
    port->setSpeed(GetPortSpeed(port->getID()));
  }
  warmBootState->resetIntfs(warmBootCache_->reconstructInterfaceMap());
  warmBootState->resetVlans(warmBootCache_->reconstructVlanMap());
  return warmBootState;
}

std::pair<std::shared_ptr<SwitchState>, BootType>
SaiSwitch::init(HwSwitch::Callback *callback) {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto state = make_shared<SwitchState>();
  auto bootType = BootType::COLD_BOOT;
  
  CHECK(bootType != BootType::UNINITIALIZED);
  auto warmBoot = bootType == BootType::WARM_BOOT;
  
  char devId = 0;
  callback_ = callback;

  sai_switch_profile_id_t profileId = SAI_SWITCH_DEFAULT_PROFILE_ID;

  // environment variable exists ?
  if (NULL != std::getenv("SAI_SWITCH_PROFILE_ID"))
  {
    // get profile ID from variable value
    profileId = atoi(std::getenv("SAI_SWITCH_PROFILE_ID"));
  }

  // environment variable exists ?
  if (NULL != std::getenv("SAI_SWITCH_HARDWARE_ID"))
  {
    // get switch hardware ID from variable value
    hwId_ = std::string(std::getenv("SAI_SWITCH_HARDWARE_ID"));
  }

  sai_switch_notification_t swNotif = {NULL, NULL, NULL, NULL, NULL, NULL};
  swNotif.on_packet_event = PacketRxCallback;

  sai_status_t saiStatus = saiSwitchApi_->initialize_switch(profileId,
                                                             const_cast<char*>(hwId_.c_str()),
                                                             NULL, &swNotif);
  if(SAI_STATUS_SUCCESS != saiStatus) {
    throw SaiFatal("Failed to initialize SAI switch. Error: ", saiStatus);
  }

  sai_attribute_t attr;
  std::vector<sai_attribute_t> hostIfAttrList;

  // Create a host interface
  attr.id = SAI_HOSTIF_ATTR_TYPE;
  attr.value.s32 = SAI_HOSTIF_TYPE_FD;
  hostIfAttrList.push_back(attr);

  saiStatus = saiHostIntfApi_->create_hostif(&hostIfFdId_,
                                              hostIfAttrList.size(),
                                              hostIfAttrList.data());
  if(SAI_STATUS_SUCCESS != saiStatus) {
    throw SaiFatal("Failed to initialize SAI host interface. Error: ", saiStatus);
  }

  attr.id = SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION;
  attr.value.s32 = SAI_PACKET_ACTION_TRAP;

  saiStatus = saiHostIntfApi_->set_trap_attribute(SAI_HOSTIF_TRAP_ID_ARP_REQUEST, &attr);
  if(SAI_STATUS_SUCCESS != saiStatus) {
    throw SaiFatal("Could not set ARP_REQUEST trap action to LOG. Error: ", saiStatus);
  }

  saiStatus = saiHostIntfApi_->set_trap_attribute(SAI_HOSTIF_TRAP_ID_ARP_RESPONSE, &attr);
  if(SAI_STATUS_SUCCESS != saiStatus) {
    throw SaiFatal("Could not set ARP_RESPONSE trap action to LOG. Error: ", saiStatus);
  }

  if(!warmBoot){
    bootType = BootType::COLD_BOOT;

    attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;
    saiStatus = GetSaiSwitchApi()->get_switch_attribute(1, &attr);

    if(SAI_STATUS_SUCCESS != saiStatus)
      throw SaiFatal("Retrieve port number error.");

    saiPortList_.list = new sai_object_id_t[attr.value.u32];
    saiPortList_.count = attr.value.u32;
    attr.value.objlist = saiPortList_;

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    saiStatus = GetSaiSwitchApi()->get_switch_attribute(1, &attr);

    if(SAI_STATUS_SUCCESS != saiStatus)
      throw SaiFatal("Retrieve port list error.");

    VLOG(2) << "Performing cold boot";
  }
  else {
     VLOG(2) << "Performing warm boot";
  }
  portTable_->InitPorts(false);
  // Set the spanning tree state of all ports to forwarding.
  // TODO: Eventually the spanning tree state should be part of the Port
  // state, and this should be handled in applyConfig().
  //
  // Spanning tree group settings
  // TODO: This should eventually be done as part of applyConfig()
  
  attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
  attr.value.mac[0] = 0;
  attr.value.mac[1] = 1;
  attr.value.mac[2] = 2;
  attr.value.mac[3] = 3;
  attr.value.mac[4] = 4;
  attr.value.mac[5] = 5;
  saiStatus = GetSaiSwitchApi()->set_switch_attribute(&attr);

  if(SAI_STATUS_SUCCESS != saiStatus)
    throw SaiFatal("Set switch MAC address error.");
    
  //TODO: Create file to indicate init state
  TryGetLock();
  if (warmBoot) {
    state = GetWarmBootSwitchState();
    stateChanged(StateDelta(make_shared<SwitchState>(), state));
  }
  else
    state = GetColdBootSwitchState();

  return std::make_pair(state, bootType);
}

void SaiSwitch::unregisterCallbacks() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

void SaiSwitch::EcmpHashSetup() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

void SaiSwitch::stateChanged(const StateDelta &delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  try{

    // Take the lock before modifying any objects
    std::lock_guard<std::mutex> g(lock_);
    // As the first step, disable ports that are now disabled.
    // This ensures that we immediately stop forwarding traffic on these ports.
    processDisabledPorts(delta);

    // remove all routes to be deleted
    ProcessRemovedRoutes(delta);

    // delete all interface not existing anymore. that should stop
    // all traffic on that interface now
    forEachRemoved(delta.getIntfsDelta(), &SaiSwitch::ProcessRemovedIntf, this);

    // Add all new VLANs, and modify VLAN port memberships.
    // We don't actually delete removed VLANs at this point, we simply remove
    // all members from the VLAN.  This way any ports that ingres packets to this
    // VLAN will still switch use this VLAN until we get the new VLAN fully
    // configured.
    forEachChanged(delta.getVlansDelta(),
                   &SaiSwitch::ProcessChangedVlan,
                   &SaiSwitch::ProcessAddedVlan,
                   &SaiSwitch::PreprocessRemovedVlan,
                   this);

    // Edit port ingress VLAN and speed settings
    forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port> &oldPort, const shared_ptr<Port> &newPort) {
      if (oldPort->getIngressVlan() != newPort->getIngressVlan()) {
        UpdateIngressVlan(oldPort, newPort);
      }

      if (oldPort->getSpeed() != newPort->getSpeed()) {
        UpdatePortSpeed(oldPort, newPort);
      }
    });

    // Update changed interfaces
    forEachChanged(delta.getIntfsDelta(), &SaiSwitch::ProcessChangedIntf, this);

    // Remove deleted VLANs
    forEachRemoved(delta.getVlansDelta(), &SaiSwitch::ProcessRemovedVlan, this);

    // Add all new interfaces
    forEachAdded(delta.getIntfsDelta(), &SaiSwitch::ProcessAddedIntf, this);

    // Any ARP changes
    ProcessArpChanges(delta);

    // Process any new routes or route changes
    ProcessAddedChangedRoutes(delta);

    // As the last step, enable newly enabled ports.
    // Doing this as the last step ensures that we only start forwarding traffic
    // once the ports are correctly configured.
    processEnabledPorts(delta);

  } catch(SaiError &e) {
    LOG(ERROR) << e.what();
  } catch(SaiFatal &e) {
    LOG(FATAL) << e.what();
    exitFatal();
  }
}

std::unique_ptr<TxPacket> SaiSwitch::allocatePacket(uint32_t size) {
  VLOG(5) << "Entering " << __FUNCTION__;

  return make_unique<SaiTxPacket>(size);
}

bool SaiSwitch::sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept {
  VLOG(5) << "Entering " << __FUNCTION__;

  sai_attribute_t attr;
  std::vector<sai_attribute_t> attrList;

  attr.id = SAI_HOSTIF_PACKET_TX_TYPE;
  attr.value.s32 = SAI_HOSTIF_TX_TYPE_PIPELINE_LOOKUP;
  attrList.push_back(attr);

  sai_status_t status = saiHostIntfApi_->send_packet(hostIfFdId_,
                                                      pkt->buf()->writableData(), 
                                                      pkt->buf()->length(),
                                                      attrList.size(),
                                                      attrList.data());
  if (status != SAI_STATUS_SUCCESS) {
    LOG(ERROR) << "Failed to send switched packet. Error: " << status;
    return false;
  }

  return true;
}

bool SaiSwitch::sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt, PortID portID) noexcept {

  sai_object_id_t portId {SAI_NULL_OBJECT_ID};

  try {
    portId = GetPortTable()->GetSaiPortId(portID);
  } catch (const SaiError &e) {
    LOG(ERROR) << "Could not sent packet out of port:" << portID.t 
               << " Reason: " << e.what();
    return false;
  }

  sai_attribute_t attr;
  std::vector<sai_attribute_t> attrList;

  attr.id = SAI_HOSTIF_PACKET_TX_TYPE;
  attr.value.s32 = SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS;
  attrList.push_back(attr);

  attr.id = SAI_HOSTIF_PACKET_EGRESS_PORT_OR_LAG;
  attr.value.oid = portId;
  attrList.push_back(attr);

  sai_status_t status = saiHostIntfApi_->send_packet(hostIfFdId_,
                                                      pkt->buf()->writableData(), 
                                                      pkt->buf()->length(),
                                                      attrList.size(),
                                                      attrList.data());
  if (status != SAI_STATUS_SUCCESS) {
    LOG(ERROR) << "Failed to send packet out of port: " << portID.t << " Error: " << status;
    return false;
  }

  return true;
}

void SaiSwitch::UpdateIngressVlan(const std::shared_ptr<Port> &oldPort,
                                  const std::shared_ptr<Port> &newPort) {
  VLOG(4) << "Entering " << __FUNCTION__;

  try {
    portTable_->GetSaiPort(newPort->getID())->SetIngressVlan(newPort->getIngressVlan());
  } catch (const SaiError &e) {
    LOG(ERROR) << e.what();
  }
}

void SaiSwitch::UpdatePortSpeed(const std::shared_ptr<Port> &oldPort,
                                const std::shared_ptr<Port> &newPort) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

void SaiSwitch::processDisabledPorts(const StateDelta& delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      if (!oldPort->isDisabled() && newPort->isDisabled()) {

        try {
          portTable_->GetSaiPort(newPort->getID())->disable();
        } catch (const SaiError &e) {
          LOG(ERROR) << e.what();
        }
      }
    });
}

void SaiSwitch::processEnabledPorts(const StateDelta& delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {

      if (oldPort->isDisabled() && !newPort->isDisabled()) {

        try {
          portTable_->GetSaiPort(newPort->getID())->enable();
        } catch (const SaiError &e) {
          LOG(ERROR) << e.what();
        }
      }

    });
}

void SaiSwitch::clearWarmBootCache() {
  VLOG(4) << "Entering " << __FUNCTION__;
  
  warmBootCache_->clear();
}

void SaiSwitch::exitFatal() const {
  VLOG(4) << "Entering " << __FUNCTION__;
  LOG(FATAL) << "Exit fatal";
  // TODO: dump state

  // report
  callback_->exitFatal();
}

folly::dynamic SaiSwitch::gracefulExit() {
  VLOG(4) << "Entering " << __FUNCTION__;
  LOG(WARNING) << "Exit graceful";
  folly::dynamic hwSwitch = toFollyDynamic();
  // TODO: Save cache for next warm boot
  GetSaiSwitchApi()->disconnect_switch();
  return hwSwitch;
}

folly::dynamic SaiSwitch::toFollyDynamic() const {
  VLOG(4) << "Entering " << __FUNCTION__;
  folly::dynamic hwSwitch = folly::dynamic::object;

  return hwSwitch;
}

void SaiSwitch::initialConfigApplied() {
  VLOG(4) << "Entering " << __FUNCTION__;
  // TODO:
}

bool SaiSwitch::isPortUp(PortID port) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  //TODO:
  return true;
}

void SaiSwitch::updateStats(SwitchStats *switchStats) {
  VLOG(4) << "Entering " << __FUNCTION__;

  UpdateSwitchStats(switchStats);

  // Update thread-local per-port statistics.
  PortStatsMap *portStatsMap = switchStats->getPortStats();

  for (auto& entry : *portStatsMap) {
    PortID portId = entry.first;
    PortStats *portStats = entry.second.get();
    UpdatePortStats(portId, portStats);
  }

  // Update global statistics.
  portTable_->UpdatePortStats();
}

int SaiSwitch::getHighresSamplers(
  HighresSamplerList *samplers,
  const folly::StringPiece namespaceString,
  const std::set<folly::StringPiece> &counterSet) {
  return 0;
}

bool SaiSwitch::getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) {
  VLOG(4) << "Entering " << __FUNCTION__;
  return false;
}

void SaiSwitch::fetchL2Table(std::vector<L2EntryThrift> *l2Table) {
}

cfg::PortSpeed SaiSwitch::getPortSpeed(PortID port) const {
  VLOG(4) << "Entering " << __FUNCTION__;
  // TODO: Get the actual port speed. 
  return cfg::PortSpeed(10000);
}

cfg::PortSpeed SaiSwitch::getMaxPortSpeed(PortID port) const {
  VLOG(4) << "Entering " << __FUNCTION__;
  // TODO: Get the actual max port speed. 
  return cfg::PortSpeed(10000);
}

void SaiSwitch::UpdateSwitchStats(SwitchStats *switchStats) {
  VLOG(4) << "Entering " << __FUNCTION__;
  // TODO
}

void SaiSwitch::UpdatePortStats(PortID portID, PortStats *portStats) {
  VLOG(4) << "Entering " << __FUNCTION__;
  // TODO
}

SaiPlatformBase *SaiSwitch::GetPlatform() const {
  VLOG(4) << "Entering " << __FUNCTION__;

  return platform_;
}

template<typename DELTA>
void SaiSwitch::ProcessNeighborEntryDelta(const DELTA &delta) {
  VLOG(4) << "Entering " << __FUNCTION__;
      
  const auto *oldEntry = delta.getOld().get();
  const auto *newEntry = delta.getNew().get();

  auto updateCreateNewEntry = [&]() {
    sai_packet_action_t action = newEntry->isPending() ?
                                   SAI_PACKET_ACTION_DROP :
                                   SAI_PACKET_ACTION_FORWARD;

    VLOG(3) << "Adding neighbor entry witch action: " << action;
    auto host = hostTable_->createOrUpdateSaiHost(newEntry->getIntfID(),
                                                  IPAddress(newEntry->getIP()),
                                                  newEntry->getMac());
    try {
      auto oldAction = host->getSaiAction();

      host->Program(action, newEntry->getMac());

      if ((oldAction != action) &&
          (action == SAI_PACKET_ACTION_FORWARD)) {

        // The host has been just resolved so update 
        // dependant Next Hops and Routes correspondingly 
        nextHopTable_->onResolved(newEntry->getIntfID(), newEntry->getIP());
        routeTable_->onResolved(newEntry->getIntfID(), newEntry->getIP());
      }
    } catch (const SaiError &e) {
      hostTable_->removeSaiHost(newEntry->getIntfID(),
                                IPAddress(newEntry->getIP()));
      LOG(ERROR) << e.what();
      return;
    }

  };

  auto removeOldEntry = [&]() {
    hostTable_->removeSaiHost(oldEntry->getIntfID(),
                              IPAddress(oldEntry->getIP()));
  };

  if (!oldEntry) {
    updateCreateNewEntry();

  } else if (!newEntry) {

    removeOldEntry();

  } else if ((oldEntry->getIntfID() != newEntry->getIntfID()) ||
             (oldEntry->getIP() != newEntry->getIP())) {

    updateCreateNewEntry();
    removeOldEntry();

  } else if (oldEntry->getMac() != newEntry->getMac()) {

    updateCreateNewEntry();
  }
}

void SaiSwitch::ProcessArpChanges(const StateDelta &delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  for (const auto& vlanDelta : delta.getVlansDelta()) {
    for (const auto& arpDelta : vlanDelta.getArpDelta()) {
      ProcessNeighborEntryDelta(arpDelta);
    }

    for (const auto& ndpDelta : vlanDelta.getNdpDelta()) {
      ProcessNeighborEntryDelta(ndpDelta);
    }
  }
}

template <typename RouteT>
void SaiSwitch::ProcessChangedRoute(const RouterID id,
                                    const shared_ptr<RouteT> &oldRoute,
                                    const shared_ptr<RouteT> &newRoute) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(2) << "Changing route entry vrf: " << (int)id << ", from "
          << oldRoute->str() << " to " << newRoute->str();
    
  // if the new route is not resolved, delete it instead of changing it
  if (!newRoute->isResolved()) {
    VLOG(2) << "Non-resolved route HW programming is skipped";
    ProcessRemovedRoute(id, oldRoute);
  } else {
    try {
      routeTable_->AddRoute(id, newRoute.get());
    } catch (const SaiError &e) {
      LOG(ERROR) << e.what();
    }
  }
}

template <typename RouteT>
void SaiSwitch::ProcessAddedRoute(const RouterID id,
                                  const shared_ptr<RouteT> &route) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(2) << "Adding route entry vrf: " << (int)id << ", " << route->str();

  // if the new route is not resolved, ignore it
  if (!route->isResolved()) {
    VLOG(2) << "Non-resolved route HW programming is skipped";
    return;
  }

  try {
    routeTable_->AddRoute(id, route.get());
  } catch (const SaiError &e) {
    LOG(ERROR) << e.what();
  }
}

template <typename RouteT>
void SaiSwitch::ProcessRemovedRoute(const RouterID id,
                                    const shared_ptr<RouteT> &route) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(3) << "removing route entry @ vrf " << id << " " << route->str();

  if (!route->isResolved()) {
    VLOG(1) << "Non-resolved route HW programming is skipped";
    return;
  }

  try {
    routeTable_->DeleteRoute(id, route.get());
  } catch (const SaiError &e) {
    LOG(ERROR) << e.what();
  }
}

void SaiSwitch::ProcessRemovedRoutes(const StateDelta &delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getOld()) {
      // no old route table, must not removed route, skip
      continue;
    }

    RouterID id = rtDelta.getOld()->getID();
    forEachRemoved(
      rtDelta.getRoutesV4Delta(),
      &SaiSwitch::ProcessRemovedRoute<RouteV4>,
      this,
      id);
    forEachRemoved(
      rtDelta.getRoutesV6Delta(),
      &SaiSwitch::ProcessRemovedRoute<RouteV6>,
      this,
      id);
  }
}

void SaiSwitch::ProcessAddedChangedRoutes(const StateDelta &delta) {
  VLOG(4) << "Entering " << __FUNCTION__;

  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getNew()) {
      // no new route table, must not added or changed route, skip
      continue;
    }

    RouterID id = rtDelta.getNew()->getID();
    
    forEachChanged(
      rtDelta.getRoutesV4Delta(),
      &SaiSwitch::ProcessChangedRoute<RouteV4>,
      &SaiSwitch::ProcessAddedRoute<RouteV4>,
      [&](SaiSwitch *, RouterID, const shared_ptr<RouteV4> &) {},
      this,
      id);
    
    forEachChanged(
      rtDelta.getRoutesV6Delta(),
      &SaiSwitch::ProcessChangedRoute<RouteV6>,
      &SaiSwitch::ProcessAddedRoute<RouteV6>,
      [&](SaiSwitch *, RouterID, const shared_ptr<RouteV6> &) {},
      this,
      id);
  }
}

void SaiSwitch::ProcessChangedVlan(const shared_ptr<Vlan> &oldVlan,
                                   const shared_ptr<Vlan> &newVlan) {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiStatus = SAI_STATUS_SUCCESS;
  sai_vlan_id_t vlanId = newVlan->getID();
  std::vector<sai_vlan_port_t> addedPorts;
  std::vector<sai_vlan_port_t> removedPorts;
  const auto &oldPorts = oldVlan->getPorts();
  const auto &newPorts = newVlan->getPorts();

  // Populate the list of removed ports
  for (auto& oldPortPair : oldPorts) {
    if (newPorts.find(oldPortPair.first) == newPorts.end()) {

      sai_vlan_port_t vlanPort;

      try {
        vlanPort.port_id = portTable_->GetSaiPortId(oldPortPair.first);
      } catch (const SaiError &e) {
        LOG(ERROR) << e.what();
        continue;
      }

      vlanPort.tagging_mode = oldPortPair.second.tagged ? SAI_VLAN_PORT_TAGGED :
                                                          SAI_VLAN_PORT_UNTAGGED;
      removedPorts.push_back(vlanPort);
    }
  }

  // Populate the list of added ports
  for (auto& newPortPair : newPorts) {
    if (oldPorts.find(newPortPair.first) == oldPorts.end()) {

      sai_vlan_port_t vlanPort;

      try {
        vlanPort.port_id = portTable_->GetSaiPortId(newPortPair.first);
      } catch (const SaiError &e) {
        LOG(ERROR) << e.what();
        continue;
      }

      vlanPort.tagging_mode = newPortPair.second.tagged ? SAI_VLAN_PORT_TAGGED :
                                                          SAI_VLAN_PORT_UNTAGGED;
      addedPorts.push_back(vlanPort);
    }
  }

  if (removedPorts.size() > 0) {

    VLOG(2) << "updating VLAN " << newVlan->getID() << ": "  << removedPorts.size() << " ports removed";

    saiStatus = saiVlanApi_->remove_ports_from_vlan(vlanId, removedPorts.size(),
                                                     removedPorts.data());
    if (saiStatus != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Failed to remove ports from VLAN " << vlanId;
    }
  }

  if (addedPorts.size() > 0) {

    VLOG(2) << "updating VLAN " << newVlan->getID() << ": "  << addedPorts.size() << " ports added";

    saiStatus = saiVlanApi_->add_ports_to_vlan(vlanId, addedPorts.size(),
                                                addedPorts.data());
    if (saiStatus != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Failed to add ports to VLAN " << vlanId;
    }
  }
}

void SaiSwitch::ProcessAddedVlan(const shared_ptr<Vlan> &vlan) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(3) << "Creating VLAN " << (uint16_t)vlan->getID() << " with "
          << vlan->getPorts().size() << " ports."; 

  sai_status_t saiStatus;
  sai_vlan_id_t vlanId = vlan->getID();
  std::vector<sai_vlan_port_t> portList;

  for (const auto& entry : vlan->getPorts()) {
    sai_vlan_port_t vlanPort;

    try {
      vlanPort.port_id = portTable_->GetSaiPortId(entry.first);
    } catch (const SaiError &e) {
      LOG(ERROR) << e.what();
      continue;
    }

    vlanPort.tagging_mode = entry.second.tagged ? SAI_VLAN_PORT_TAGGED :
                                                  SAI_VLAN_PORT_UNTAGGED;
    portList.push_back(vlanPort);
  }

  typedef SaiWarmBootCache::VlanInfo VlanInfo;
  // Since during warm boot all VLAN in the config will show
  // up as added VLANs we only need to consult the warm boot
  // cache here.
  auto vlanItr = warmBootCache_->findVlanInfo(vlan->getID());
  if (vlanItr != warmBootCache_->vlan2VlanInfo_end()) {

    // Compare with existing vlan to determine if we need to reprogram
    auto is_equal = [](VlanInfo newVlan, VlanInfo existingVlan) {

      auto newVPorts = newVlan.ports_;
      auto existingVPorts = existingVlan.ports_;

      if((newVlan.vlan_ != existingVlan.vlan_) ||
         (newVPorts.size() != existingVPorts.size())) {

        return false;
      }

      for(uint32_t i = 0; i < newVPorts.size(); ++i) {

        if((newVPorts[i].port_id != existingVPorts[i].port_id) ||
           (newVPorts[i].tagging_mode != existingVPorts[i].tagging_mode)) {

          return false;
        }
      }

      return true;
    };
    
    const auto& existingVlan = vlanItr->second;

    if (!is_equal(VlanInfo(vlan->getID(), portList), existingVlan)) {

      VLOG(3) << "Updating VLAN " << (uint16_t)vlan->getID() << " with "
              << vlan->getPorts().size() << " ports.";
      
      auto oldVlan = vlan->clone();
      warmBootCache_->fillVlanPortInfo(oldVlan.get());
      ProcessChangedVlan(oldVlan, vlan);
    } else {
      LOG(WARNING) << "Vlan " << vlan->getID() << " already exists.";
    }
    
    warmBootCache_->programmed(vlanItr);

  } else {
    VLOG(3) << "Creating VLAN " << (uint16_t)vlan->getID() << " with "
            << vlan->getPorts().size() << " ports.";

    saiStatus = saiVlanApi_->create_vlan(vlanId);
    if (saiStatus != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Failed to create VLAN " << vlanId;
      return;
    }

    saiStatus = saiVlanApi_->add_ports_to_vlan(vlanId, portList.size(), portList.data());
    if (saiStatus != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Failed to add ports to VLAN " << vlanId;
      return;
    }

    warmBootCache_->AddVlanInfo(vlan->getID(), portList);
  }
}

void SaiSwitch::PreprocessRemovedVlan(const shared_ptr<Vlan> &vlan) {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiStatus = SAI_STATUS_SUCCESS;
  sai_vlan_id_t vlanId = vlan->getID();
  std::vector<sai_vlan_port_t> portList;

  for (const auto& entry : vlan->getPorts()) {
    sai_vlan_port_t vlanPort;

    try {
      vlanPort.port_id = portTable_->GetSaiPortId(entry.first);
    } catch (const SaiError &e) {
      LOG(ERROR) << e.what();
      continue;
    }

    vlanPort.tagging_mode = entry.second.tagged ? SAI_VLAN_PORT_TAGGED :
                                                  SAI_VLAN_PORT_UNTAGGED;
    portList.push_back(vlanPort);
  }

  saiStatus = saiVlanApi_->remove_ports_from_vlan(vlanId, portList.size(), portList.data());
  if (saiStatus != SAI_STATUS_SUCCESS) {
    LOG(ERROR) << "Failed to remove VLAN " << vlanId;
  }

  saiStatus = saiVlanApi_->remove_vlan(vlanId);
  if (saiStatus != SAI_STATUS_SUCCESS) {
    LOG(ERROR) << "Failed to remove VLAN " << vlanId;
  }
}

void SaiSwitch::ProcessRemovedVlan(const shared_ptr<Vlan> &vlan) {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiStatus = SAI_STATUS_SUCCESS;

  VLOG(2) << "removing VLAN " << vlan->getID();

  //saiStatus = saiVlanApi_->remove_vlan(vlan->getID());

  if (saiStatus != SAI_STATUS_SUCCESS)
    throw SaiError("Failed to remove VLAN ", vlan->getID());
}

void SaiSwitch::ProcessChangedIntf(const shared_ptr<Interface> &oldIntf,
                                   const shared_ptr<Interface> &newIntf) {
  VLOG(4) << "Entering " << __FUNCTION__;

  CHECK_EQ(oldIntf->getID(), newIntf->getID());
  VLOG(2) << "changing interface " << oldIntf->getID();
  intfTable_->ProgramIntf(newIntf);
}

void SaiSwitch::ProcessAddedIntf(const shared_ptr<Interface> &intf) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(2) << "adding interface " << intf->getID();
  try {
    intfTable_->AddIntf(intf);
  } catch (const SaiError &e) {
    LOG(ERROR) << e.what();
    return;
  }
}

void SaiSwitch::ProcessRemovedIntf(const shared_ptr<Interface> &intf) {
  VLOG(4) << "Entering " << __FUNCTION__;

  VLOG(2) << "deleting interface " << intf->getID();
  intfTable_->DeleteIntf(intf);
}

#define lockPath  "sai_agent.lock"
int SaiSwitch::TryGetLock(){
  VLOG(4) << "Entering " << __FUNCTION__;

  if(lockFd != -1)
    return lockFd;

  mode_t mode = umask(0);
  lockFd = open(lockPath, O_RDWR|O_CREAT, 0666 );
  umask(mode);
  
  if(lockFd >= 0 && flock(lockFd, LOCK_EX | LOCK_NB ) < 0)
  {
    close( lockFd );
    lockFd = -1;
  }
  
  return lockFd;
}

void SaiSwitch::PacketRxCallback(const void *buf,
                                 sai_size_t buf_size,
                                 uint32_t attr_count,
                                 const sai_attribute_t *attr_list) {

  // TODO: Get SaiSwitch from callback parameter
  // when this is supported in SAI.  
  auto* sw = SaiSwitch::instance_;

  return sw->OnPacketReceived(buf, buf_size, attr_count, attr_list);
}

void SaiSwitch::OnPacketReceived(const void *buf,
                                 sai_size_t buf_size,
                                 uint32_t attr_count,
                                 const sai_attribute_t *attr_list) noexcept{
  unique_ptr<SaiRxPacket> pkt;

  try {
    pkt = make_unique<SaiRxPacket>(buf, buf_size, attr_count, attr_list, this);
  } catch (const SaiError &e) {

    LOG(ERROR) << __FUNCTION__ << " Could not allocate SaiRxPacket. Reason: "
      << e.what();
    return;

  } catch (const std::exception &ex) {

    LOG(ERROR) << __FUNCTION__ << " Could not allocate SaiRxPacket. Reason: "
               << folly::exceptionStr(ex);
    return;
  }

  callback_->packetReceived(std::move(pkt));
}

void SaiSwitch::ReleaseLock()
{
  VLOG(4) << "Entering " << __FUNCTION__;
  
  if(lockFd < 0)
    return;
  remove(lockPath);
  close(lockFd);
}

}} // facebook::fboss
