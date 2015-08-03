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
#include "fboss/agent/state/Interface.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TEventHandler.h"

namespace facebook { namespace fboss {

class SwSwitch;
class RxPacket;

class TunIntf : private apache::thrift::async::TEventHandler {
 public:
  TunIntf(SwSwitch *sw, apache::thrift::async::TEventBase *evb,
          const std::string& name, RouterID rid, int idx, int mtu);
  TunIntf(SwSwitch *sw, apache::thrift::async::TEventBase *evb,
          RouterID rid, const Interface::Addresses& addrs, int mtu);
  ~TunIntf() override;

  // some utility functions
  static bool isTunIntf(const char *name);
  static RouterID getRidFromName(const char *name);

  int getIfIndex() const {
    return ifIndex_;
  }
  const std::string& getName() const {
    return name_;
  }
  const Interface::Addresses& getAddresses() const {
    return addrs_;
  }
  RouterID getRouterId() const {
    return rid_;
  }
  /**
   * Mark delete of the interface from the host. The interface on the host
   * will be deleted in destructor after the mark.
   */
  void setDelete() {
    toDelete_ = true;
  }
  /**
   * Add a discovered address to the interface.
   * This function just modifies the software object.
   */
  void addAddress(const folly::IPAddress& addr, uint8_t mask);
  /**
   * Set the new addresses for the interface.
   * This function just modifies the software object.
   */
  void setAddresses(const Interface::Addresses& addrs) {
    addrs_ = addrs;
  }
  /// Start packet forwarding.
  void start();
  /// Stop packet forwarding.
  void stop();
  void handlerReady(uint16_t events) noexcept override;
  /**
   * Send a packet to the interface on host.
   * Unlike other methods, which are called on thread that serves the evb,
   * this function can be called from any thread.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);

  /// Set the maximum MTU
  void setMtu(int mtu);
  int getMtu(int mtu) {
    return mtu_;
  }
 private:
  SwSwitch *sw_;
  RouterID rid_;         ///< The router ID of the interface belonging to
  std::string name_;    ///< The name in the host
  int ifIndex_{-1};     ///< The ifindex of the interface.
  bool toDelete_{false}; ///< Is the interface to be deleted from system
  Interface::Addresses addrs_; ///< The IP addresses assigned to this intf
  int mtu_;
  /**
   * File descriptor for this interface through which packets can
   * be received from or sent to.
   */
  int fd_{-1};
  int mtu_{-1};

  std::string makeIntfName(RouterID rid);
  void openFD();
  void closeFD() noexcept;
};

}}
