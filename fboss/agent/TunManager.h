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
#include "thrift/lib/cpp/async/TEventBase.h"

#include <boost/container/flat_map.hpp>

extern "C" {
#include <libnetlink.h>
}

namespace facebook { namespace fboss {

class InterfaceMap;
class RxPacket;
class SwSwitch;
class TunIntf;

class TunManager : public StateObserver {
 public:
  TunManager(SwSwitch *sw, apache::thrift::async::TEventBase *evb);
  ~TunManager() override;
  /**
   * Start probe procedure to read TUN interface info from the host.
   * This function can be called from any thread.
   * The probe function will happen in the thread serving 'evb_'
   */
  void startProbe();
  /**
   * Update the intfs_ map based on the given state update. This
   * overrides the StateObserver stateUpdated api, which is always
   * guaranteed to be called from the update thread.
   */
  void stateUpdated(const StateDelta& delta) override;

  void startObservingUpdates();
  /**
   * Send a packet to host.
   * This function can be called from any thread.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);
  /*
   * Sync to SwitchState
   * This should really be only called externally
   * once, after config is applied. After that all
   * updates should come via the stateUpdated calls
   */
  void sync(std::shared_ptr<SwitchState> state);

 private:
  // no copy to assign
  TunManager(const TunManager &) = delete;
  TunManager& operator=(const TunManager &) = delete;

  SwSwitch *sw_;
  apache::thrift::async::TEventBase *evb_;
  boost::container::flat_map<RouterID, std::unique_ptr<TunIntf>> intfs_;
  rtnl_handle rth_;
  /**
   * The mutex used to protect intfs_.
   * probe() and sync() could manipulate intfs_. They both run on the same
   * thread that serves evb_.
   * sendPacketToHost() uses intfs_, it can be called from any thread.
   */
  std::mutex mutex_;

  // Whether the manager has registered itself to listen for state updates
  // from sw_
  bool observingState_{false};

  enum : uint8_t {
    /**
     * The protocol value used to add the source routing IP rule and the
     * default route in the routing table for source routing.
     * Per rtnetlink.h, any value >= RTPROTO_STATIC(4) to identify us
     */
    RTPROT_FBOSS = 80,
  };
  /// Add a TUN interface. It is called during probe process.
  void addIntf(RouterID rid, const std::string& name, int IfIdx);
  /// Add a TUN interface. It is called to create a new TUN interface.
  void addIntf(RouterID rid, const Interface::Addresses& addrs);
  /// Remove an existing TUN interface
  void removeIntf(RouterID rid);
  /// A tun interface was changed, update the addresses accordingly
  void updateIntf(RouterID rid, const Interface::Addresses& addrs);
 /// Add an address to a TUN interface during probe process.
  void addProbedAddr(int ifIndex, const folly::IPAddress& addr, uint8_t mask);
  /// Bring up the interface on the host
  void bringupIntf(const std::string& name, int ifIndex);
  /// Retrieve the route table ID based on the router ID
  int getTableId(RouterID rid) const;
  /// Add/remove a route table
  void addRemoveTable(int ifIdx, RouterID rid, bool add);
  void addRouteTable(int ifIdx, RouterID rid) {
    addRemoveTable(ifIdx, rid, true);
  }
  void removeRouteTable(int ifIdx, RouterID rid) {
    addRemoveTable(ifIdx, rid, false);
  }

  /**
   * Add/remove an IP rule for source routing based on a given address
   *
   * The default route uses the management interface as the egress interface.
   * Source routing is used to force the packets with source IP address matching
   * one the front panel port IP addresses to be sent through the front panel
   * ports instead of the management interface.
   */
  void addRemoveSourceRouteRule(RouterID rid, folly::IPAddress addr,
                                bool add);
  /// Add/remove an address to/from a TUN interface on the host
  void addRemoveTunAddress(const std::string& name, uint32_t ifIndex,
                           folly::IPAddress addr, uint8_t mask, bool add);
  void addTunAddress(const std::string& name, RouterID rid, uint32_t ifIndex,
                     folly::IPAddress addr, uint8_t mask);
  void removeTunAddress(const std::string& name, RouterID rid, uint32_t ifIndex,
                        folly::IPAddress addr, uint8_t mask);
  /// The callback function to parse the RTM_GETADDRrequest
  static int getAddrRespParser(const struct sockaddr_nl *who,
                               struct nlmsghdr *n, void *arg);
  /// The callback function to parse the RTM_GETLINK request
  static int getLinkRespParser(const struct sockaddr_nl *who,
                               struct nlmsghdr *n, void *arg);

  template<typename MAPNAME,
           typename CHANGEFN, typename ADDFN, typename REMOVEFN>
  void applyChanges(const MAPNAME& oldMap, const MAPNAME& newMap,
                    CHANGEFN changeFn, ADDFN addFn, REMOVEFN removeFn);

  void probe();
  /*
   * start/stop packet forwarding on all TUN interfaces
   */
  void stop() const;
  void start() const;
};

}}
