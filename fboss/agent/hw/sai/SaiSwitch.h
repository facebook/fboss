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
  std::shared_ptr<SwitchState> GetColdBootSwitchState() const;
  /*
   * Get state switch is in on a warm boot.
   */
  std::shared_ptr<SwitchState> GetWarmBootSwitchState() const;

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

  SaiPlatformBase *GetPlatform() const;

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

  SaiHostTable *WritableHostTable() const {
    return hostTable_.get();
  }

  SaiVrfTable *WritableVrfTable() const {
    return vrfTable_.get();
  }

  SaiNextHopTable *WritableNextHopTable() const {
    return nextHopTable_.get();
  }

  const SaiPortTable *GetPortTable() const {
    return portTable_.get();
  }
  const SaiIntfTable *GetIntfTable() const {
    return intfTable_.get();
  }
  const SaiHostTable *GetHostTable() const {
    return hostTable_.get();
  }

  const SaiVrfTable *GetVrfTable() const {
    return vrfTable_.get();
  }

  const SaiNextHopTable *GetNextHopTable() const {
    return nextHopTable_.get();
  }

  SaiWarmBootCache *getWarmBootCache() const {
    return warmBootCache_.get();
  }

  void exitFatal() const override;

  //This function should be called from Switch which know what port and where is located
  bool isPortUp(PortID port) const override;

  const sai_object_list_t &GetSaiPortList() const {
    return saiPortList_;
  }

  /* SAI API lists */
  sai_switch_api_t *GetSaiSwitchApi() const {
    return saiSwitchApi_;
  }

  sai_vlan_api_t *GetSaiVlanApi() const {
    return saiVlanApi_;
  }

  sai_port_api_t *GetSaiPortApi() const {
    return saiPortApi_;
  }

  sai_virtual_router_api_t *GetSaiVrfApi() const {
    return saiVrfApi_;
  }

  sai_route_api_t *GetSaiRouteApi() const {
    return saiRouteApi_;
  }

  sai_router_interface_api_t *GetSaiRouterIntfApi() const {
    return saiRouterIntfApi_;
  }

  sai_neighbor_api_t *GetSaiNeighborApi() const {
    return saiNeighborApi_;
  }

  sai_hostif_api_t *GetSaiHostInterfaceApi() const {
    return saiHostInterfaceApi_;
  }

  sai_acl_api_t *GetSaiAclApi() const {
    return saiAclApi_;
  }
  
  sai_next_hop_api_t *GetSaiNextHopApi() const {
    return saiNextHopApi_;
  }

  sai_next_hop_group_api_t *GetSaiNextHopGroupApi() const {
    return saiNextHopGroupApi_;
  }

private:
  // Forbidden copy constructor and assignment operator
  SaiSwitch(SaiSwitch const &) = delete;
  SaiSwitch &operator=(SaiSwitch const &) = delete;

  void UpdateSwitchStats(SwitchStats *switchStats);
  void UpdatePortStats(PortID portID, PortStats *portStats);

  void UpdateIngressVlan(const std::shared_ptr<Port> &oldPort,
                         const std::shared_ptr<Port> &newPort);
  void ProcessChangedVlan(const std::shared_ptr<Vlan> &oldVlan,
                          const std::shared_ptr<Vlan> &newVlan);
  void ProcessAddedVlan(const std::shared_ptr<Vlan> &vlan);
  void PreprocessRemovedVlan(const std::shared_ptr<Vlan> &vlan);
  void ProcessRemovedVlan(const std::shared_ptr<Vlan> &vlan);

  void ProcessChangedIntf(const std::shared_ptr<Interface> &oldIntf,
                          const std::shared_ptr<Interface> &newIntf);
  void ProcessAddedIntf(const std::shared_ptr<Interface> &intf);
  void ProcessRemovedIntf(const std::shared_ptr<Interface> &intf);

  void Deinit(bool warm = true) const;

  void EcmpHashSetup();

  template<typename DELTA>
  void ProcessNeighborEntryDelta(const DELTA &delta);
  void ProcessArpChanges(const StateDelta &delta);

  template <typename RouteT>
  void ProcessChangedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &oldRoute,
    const std::shared_ptr<RouteT> &newRoute);
  template <typename RouteT>
  void ProcessAddedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &route);
  template <typename RouteT>
  void ProcessRemovedRoute(
    const RouterID id, const std::shared_ptr<RouteT> &route);
  void ProcessRemovedRoutes(const StateDelta &delta);
  void ProcessAddedChangedRoutes(const StateDelta &delta);

  void UpdatePortSpeed(const std::shared_ptr<Port> &oldPort,
                       const std::shared_ptr<Port> &newPort);
  void processDisabledPorts(const StateDelta& delta);
  void processEnabledPorts(const StateDelta& delta);

  /*
   * Private callback called by the SAI API. Dispatches to
   * SaiSwitch::OnPacketReceived.
   */
  static void PacketRxCallback(const void *buf,
                               sai_size_t buf_size,
                               uint32_t attr_count,
                               const sai_attribute_t *attr_list);
  /*
   * Private callback called by SaiSwitch::packetRxCallback. Dispatches to
   * callback_->packetReceived.
   */
  void OnPacketReceived(const void *buf,
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
  int TryGetLock();
  void ReleaseLock();
};

}} // facebook::fboss
