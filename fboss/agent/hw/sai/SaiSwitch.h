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
#pragma once

#include "fboss/agent/types.h"
#include "fboss/agent/HwSwitch.h"

extern "C" {
#include "sai.h"
}

#include <folly/dynamic.h>

namespace facebook { namespace fboss {

class Port;
class PortStats;
class Vlan;
class Interface;
class SaiPlatformBase;
class SaiPortTable;
class SaiIntfTable;
class SaiHostTable;
class SaiRouteTable;
class SaiVrfTable;
class SaiNextHopTable;
class SaiWarmBootCache;

class SaiSwitch : public HwSwitch {
public:
  explicit SaiSwitch(SaiPlatformBase *platform);
  virtual ~SaiSwitch();

  /*
   * Get default state switch is in on a cold boot
   */
  std::shared_ptr<SwitchState> getColdBootSwitchState() const;
  /*
   * Get state switch is in on a warm boot.
   */
  std::shared_ptr<SwitchState> getWarmBootSwitchState() const;

  std::pair<std::shared_ptr<SwitchState>, BootType> init(Callback *callback) override;

  void unregisterCallbacks() override;

  void stateChanged(const StateDelta &delta) override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;

  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt, PortID portID) noexcept override;

  folly::dynamic gracefulExit() override;
  /*
   * SaiSwitch state as folly::dynamic 
   * For now we dump nothing. 
   */
  folly::dynamic toFollyDynamic() const override;

  void clearWarmBootCache() override;

  void initialConfigApplied() override;

  SaiPlatformBase *getPlatform() const;

  // TODO
  void updateStats(SwitchStats *switchStats) override;

  /*
  * Get SAI-specific samplers.
  *
  * @return     The number of requested counters we can handle.
  * @param[out] samplers         A vector of high-resolution samplers.  We will
  *                              append new samplers to this list.
  * @param[in]  namespaceString  A string respresentation of the current
  *                              counter namespace.
  * @param[in]  counterSet       The set of requested counters.
  */
  int getHighresSamplers(
    HighresSamplerList *samplers,
    const folly::StringPiece namespaceString,
    const std::set<folly::StringPiece> &counterSet) override;

  /* Returns true if the neighbor entry for the passed in ip
   * has been hit.
   */
  bool getAndClearNeighborHit(RouterID vrf,
                              folly::IPAddress& ip) override;

  void fetchL2Table(std::vector<L2EntryThrift> *l2Table) override;

  cfg::PortSpeed getPortSpeed(PortID port) const override;
  cfg::PortSpeed getMaxPortSpeed(PortID port) const override;

  SaiHostTable *writableHostTable() const {
    return hostTable_.get();
  }

  SaiVrfTable *writableVrfTable() const {
    return vrfTable_.get();
  }

  SaiNextHopTable *writableNextHopTable() const {
    return nextHopTable_.get();
  }

  const SaiPortTable *getPortTable() const {
    return portTable_.get();
  }
  const SaiIntfTable *getIntfTable() const {
    return intfTable_.get();
  }
  const SaiHostTable *getHostTable() const {
    return hostTable_.get();
  }

  const SaiVrfTable *getVrfTable() const {
    return vrfTable_.get();
  }

  const SaiNextHopTable *getNextHopTable() const {
    return nextHopTable_.get();
  }

  SaiWarmBootCache *getWarmBootCache() const {
    return warmBootCache_.get();
  }

  void exitFatal() const override;

  //This function should be called from Switch which know what port and where is located
  bool isPortUp(PortID port) const override;

  const sai_object_list_t &getSaiPortList() const {
    return saiPortList_;
  }

  /* SAI API lists */
  sai_switch_api_t *getSaiSwitchApi() const {
    return saiSwitchApi_;
  }

  sai_vlan_api_t *getSaiVlanApi() const {
    return saiVlanApi_;
  }

  sai_port_api_t *getSaiPortApi() const {
    return saiPortApi_;
  }

  sai_virtual_router_api_t *getSaiVrfApi() const {
    return saiVrfApi_;
  }

  sai_route_api_t *getSaiRouteApi() const {
    return saiRouteApi_;
  }

  sai_router_interface_api_t *getSaiRouterIntfApi() const {
    return saiRouterIntfApi_;
  }

  sai_neighbor_api_t *getSaiNeighborApi() const {
    return saiNeighborApi_;
  }

  sai_hostif_api_t *getSaiHostInterfaceApi() const {
    return saiHostInterfaceApi_;
  }

  sai_acl_api_t *getSaiAclApi() const {
    return saiAclApi_;
  }
  
  sai_next_hop_api_t *getSaiNextHopApi() const {
    return saiNextHopApi_;
  }

  sai_next_hop_group_api_t *getSaiNextHopGroupApi() const {
    return saiNextHopGroupApi_;
  }

private:
  // Forbidden copy constructor and assignment operator
  SaiSwitch(SaiSwitch const &) = delete;
  SaiSwitch &operator=(SaiSwitch const &) = delete;

  void updateSwitchStats(SwitchStats *switchStats);
  void updatePortStats(PortID portID, PortStats *portStats);

  void updateIngressVlan(const std::shared_ptr<Port> &oldPort,
                         const std::shared_ptr<Port> &newPort);
  void processChangedVlan(const std::shared_ptr<Vlan> &oldVlan,
                          const std::shared_ptr<Vlan> &newVlan);
  void processAddedVlan(const std::shared_ptr<Vlan> &vlan);
  void preprocessRemovedVlan(const std::shared_ptr<Vlan> &vlan);
  void processRemovedVlan(const std::shared_ptr<Vlan> &vlan);

  void processChangedIntf(const std::shared_ptr<Interface> &oldIntf,
                          const std::shared_ptr<Interface> &newIntf);
  void processAddedIntf(const std::shared_ptr<Interface> &intf);
  void processRemovedIntf(const std::shared_ptr<Interface> &intf);

  void deinit(bool warm = true) const;

  void ecmpHashSetup();

  template<typename DELTA>
  void processNeighborEntryDelta(const DELTA &delta);
  void processArpChanges(const StateDelta &delta);

  template <typename RouteT>
  void processChangedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &oldRoute,
    const std::shared_ptr<RouteT> &newRoute);
  template <typename RouteT>
  void processAddedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &route);
  template <typename RouteT>
  void processRemovedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &route);
  void processRemovedRoutes(const StateDelta &delta);
  void processAddedChangedRoutes(const StateDelta &delta);

  void updatePortSpeed(const std::shared_ptr<Port> &oldPort,
                       const std::shared_ptr<Port> &newPort);
  void processDisabledPorts(const StateDelta& delta);
  void processEnabledPorts(const StateDelta& delta);

  /*
   * Private callback called by the SAI API. Dispatches to
   * SaiSwitch::onPacketReceived.
   */
  static void packetRxCallback(const void *buf,
                               sai_size_t buf_size,
                               uint32_t attr_count,
                               const sai_attribute_t *attr_list);
  /*
   * Private callback called by SaiSwitch::packetRxCallback. Dispatches to
   * callback_->packetReceived.
   */
  void onPacketReceived(const void *buf,
                        sai_size_t buf_size,
                        uint32_t attr_count,
                        const sai_attribute_t *attr_list) noexcept;
  enum {
    SAI_SWITCH_DEFAULT_PROFILE_ID = 0
  };

  SaiPlatformBase *platform_ {nullptr};
  HwSwitch::Callback *callback_ {nullptr};

  std::unique_ptr<SaiPortTable> portTable_;
  std::unique_ptr<SaiIntfTable> intfTable_;
  std::unique_ptr<SaiHostTable> hostTable_;
  std::unique_ptr<SaiRouteTable> routeTable_;
  std::unique_ptr<SaiVrfTable> vrfTable_;
  std::unique_ptr<SaiNextHopTable> nextHopTable_;
  std::unique_ptr<SaiWarmBootCache> warmBootCache_;

  service_method_table_t service_ {nullptr, nullptr};
  std::string hwId_ {"dev0"};
  sai_object_list_t saiPortList_;
  sai_object_id_t hostIfFdId_ {SAI_NULL_OBJECT_ID};

  sai_switch_api_t *saiSwitchApi_ {nullptr};
  sai_vlan_api_t *saiVlanApi_ {nullptr};
  sai_port_api_t *saiPortApi_ {nullptr};
  sai_virtual_router_api_t *saiVrfApi_ {nullptr};
  sai_route_api_t *saiRouteApi_ {nullptr};
  sai_router_interface_api_t *saiRouterIntfApi_ {nullptr};
  sai_neighbor_api_t *saiNeighborApi_ {nullptr};
  sai_hostif_api_t *saiHostInterfaceApi_ {nullptr};
  sai_acl_api_t *saiAclApi_ {nullptr};
  sai_next_hop_api_t *saiNextHopApi_ {nullptr};
  sai_next_hop_group_api_t *saiNextHopGroupApi_ {nullptr};
  sai_hostif_api_t *saiHostIntfApi_ {nullptr};

  // Temporarry storage for SaiSwitch instance while SAI callbacks 
  // don't provide a pointer to user data.
  // TODO: remove this when SAI is updated.
  static SaiSwitch *instance_;

  std::mutex lock_;
  
  int lockFd;
  int tryGetLock();
  void releaseLock();
};

}} // facebook::fboss
