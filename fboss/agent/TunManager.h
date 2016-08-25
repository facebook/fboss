/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/types.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/Interface.h"
#include <folly/io/async/EventBase.h>

#include <boost/container/flat_map.hpp>

extern "C" {
#include <netlink/socket.h>
#include <netlink/object.h>
}

namespace facebook { namespace fboss {

class InterfaceMap;
class RxPacket;
class SwSwitch;
class TunIntf;

class TunManager : public StateObserver {
 public:
  TunManager(SwSwitch *sw, folly::EventBase *evb);
  ~TunManager() override;

  /**
   * Start probe procedure to read existing TUN interface info from the host.
   * This function can be called from any thread and probe function will happen
   * in the thread serving 'evb_'
   */
  void startProbe();

  /**
   * Update the intfs_ map based on the given state update. This
   * overrides the StateObserver stateUpdated api, which is always
   * guaranteed to be called from the update thread.
   */
  void stateUpdated(const StateDelta& delta) override;

  /**
   * Send a packet to host.
   * This function can be called from any thread.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);

  /**
   * Sync the new SwitchState
   * This should really be only called externally once, after config is applied.
   * After that all updates should come via the stateUpdated calls.
   *
   * SwSwitch calls this API when initial configuration is applied on agent
   * restart.
   */
  void sync(std::shared_ptr<SwitchState> state);

  /**
   * This should be called externally only after initial sync has been
   * performed.
   *
   * SwSwitch calls this API after calling initial sync when initial
   * configuration is applied.
   */
  void startObservingUpdates();

 private:
  // no copy to assign
  TunManager(const TunManager &) = delete;
  TunManager& operator=(const TunManager &) = delete;

  /**
   * start/stop packet forwarding on all TUN interfaces
   */
  void stop() const;
  void start() const;

  /**
   * Add a TUN interface. It can happen two ways
   * 1. During probe process when we discover existing Tun interface on linux
   * 2. When we want to create a new TUN interface in linux
   */
  void addIntf(const std::string& name, int ifIndex);
  void addIntf(InterfaceID ifID, const Interface::Addresses& addrs);

  // Remove an existing TUN interface
  void removeIntf(InterfaceID ifID);

  // A tun interface was changed, update the addresses accordingly
  void updateIntf(InterfaceID ifID, const Interface::Addresses& addrs);

  // Bring up the interface on the host
  void bringUpIntf(const std::string& ifName, int ifIndex);

  /**
   * Add/remove a route table.
   *
   * On Host we create a routing table for every switch interface which has
   * v4/v6 default route with corresponding Tun interface as nexthop. Everything
   * Forwarded to that routing table will comes out of Tun interface and it
   * will eventually go out of corresponding switch interface.
   */
  void addRemoveRouteTable(InterfaceID ifID, int ifIndex, bool add);
  void addRouteTable(InterfaceID ifID, int ifIndex) {
    addRemoveRouteTable(ifID, ifIndex, true);
  }
  void removeRouteTable(InterfaceID ifID, int ifIndex) {
    addRemoveRouteTable(ifID, ifIndex, false);
  }

  /**
   * Creates a tableId for given interface.
   */
  int getTableId(InterfaceID ifID) const;

  /**
   * Add/remove an IP rule for source routing based on a given address
   *
   * The default route uses the management interface as the egress interface.
   * Source routing is used to force the packets with source IP address matching
   * one the front panel port IP addresses to be sent through the corresponding
   * front panel ports instead of the management interface.
   */
  void addRemoveSourceRouteRule(
      InterfaceID ifID,
      const folly::IPAddress& addr,
      bool add);

  /**
   * Add/Remove an address to/from a TUN interface on the host
   */
  void addRemoveTunAddress(
      const std::string& ifName,
      uint32_t ifIndex,
      const folly::IPAddress& addr,
      uint8_t mask,
      bool add);

  /**
   * Add/Remove address as well source-routing-rule for TUN interface on host.
   */
  void addTunAddress(
      InterfaceID ifID,
      const std::string& ifName,
      uint32_t ifIndex,
      folly::IPAddress addr,
      uint8_t mask);
  void removeTunAddress(
      InterfaceID ifID,
      const std::string& ifName,
      uint32_t ifIndex,
      folly::IPAddress addr,
      uint8_t mask);

  /**
   * Netlink callback for processing and storing links
   */
  static void linkProcessor(struct nl_object *obj, void *data);

  /**
   * Netlink callback for processing and storing addresses
   */
  static void addressProcessor(struct nl_object *obj, void *data);

  /**
   * Lookup host for existing Tun interfaces and their addresses.
   */
  void probe();
  void doProbe(std::lock_guard<std::mutex>& mutex);

  /**
   * Add an address to a TUN interface during probe process.
   */
  void addProbedAddr(int ifIndex, const folly::IPAddress& addr, uint8_t mask);

  /**
   * Get MTU of switch interface
   */
  int getInterfaceMtu(InterfaceID ifID) const;

  template<typename MAPNAME,
           typename CHANGEFN, typename ADDFN, typename REMOVEFN>
  void applyChanges(const MAPNAME& oldMap, const MAPNAME& newMap,
                    CHANGEFN changeFn, ADDFN addFn, REMOVEFN removeFn);

  SwSwitch *sw_{nullptr};
  folly::EventBase *evb_{nullptr};

  // Netlink socket for managing interface/addresses in Host/Linux
  nl_sock *sock_{nullptr};

  /**
   * The mutex used to protect intfs_.
   * probe() and sync() could manipulate intfs_. They both run on the same
   * thread that serves evb_.
   * sendPacketToHost() uses intfs_, it can be called from any thread.
   */
  boost::container::flat_map<InterfaceID, std::unique_ptr<TunIntf>> intfs_;
  std::mutex mutex_;

  // Whether the manager has registered itself to listen for state updates
  // from sw_
  bool observingState_{false};

  // Initial probe done
  bool probeDone_{false};

  enum : uint8_t {
    /**
     * The protocol value used to add the source routing IP rule and the
     * default route in the routing table for source routing.
     * Per rtnetlink.h, any value >= RTPROTO_STATIC(4) to identify us
     */
    RTPROT_FBOSS = 80,
  };
};

}}  // namespace facebook::fboss
