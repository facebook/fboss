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

#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;
class RxPacket;

class TunIntf : private folly::EventHandler {
 public:
  /**
   * Creates a TunIntf object of already existing linux interface. Initial
   * status is set to `false` for discovered interfaces because we do not
   * have real port-status info. Once initial config is applied in TunManager
   * their actual status will be reflected.
   */
  TunIntf(
      SwSwitch* sw,
      folly::EventBase* evb,
      InterfaceID ifID,
      int ifIndex /* linux */,
      int mtu);

  /**
   * This version of constructor creates a Tun interface in Linux as well.
   */
  TunIntf(
      SwSwitch* sw,
      folly::EventBase* evb,
      InterfaceID ifID, // Switch interface ID
      bool status,
      const Interface::Addresses& addrs,
      int mtu);

  ~TunIntf() override;

  /**
   * Start/Stop packet forwarding on Tun interface.
   */
  void start();
  void stop();

  /**
   * Mark delete of the interface from the host. The interface on the host
   * will be deleted in destructor after the mark.
   */
  void setDelete() {
    toDelete_ = true;
  }

  /**
   * Change status of an interface UP/DOWN.
   * NOTE: This only changes the SW state.
   */
  void setStatus(bool status) {
    status_ = status;
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

  /**
   * Send a packet to the interface on host.
   * Unlike other methods, which are called on thread that serves the evb,
   * this function can be called from any thread.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);

  /**
   * Set the maximum MTU on Tun interface
   */
  void setMtu(int mtu);

  int getIfIndex() const {
    return ifIndex_;
  }

  std::string getName() const {
    return name_;
  }

  InterfaceID getInterfaceID() const {
    return ifID_;
  }

  Interface::Addresses getAddresses() const {
    return addrs_;
  }

  int getMtu() const {
    return mtu_;
  }

  bool getStatus() const {
    return status_;
  }

 private:
  /**
   * Callback for event on Tun interface's read socket-fd
   * Override's folly::EventHandler handlerReady callback.
   */
  void handlerReady(uint16_t events) noexcept override;

  /**
   * Open/Close a new socket-fd to read/write data from Tun interface.
   * fd_ is mutated.
   */
  void openFD();
  void closeFD() noexcept;

  /**
   * In newer kernel an interface is automatically gets link-local IPv6 address
   * because of IPv6 autoconf and FBOSS (we) assign one more.
   *
   * Here we disable address generation mode so that we don't end up assigning
   * address to an interface.
   */
  static void disableIPv6AddrGenMode(int ifIndex);

  SwSwitch* sw_{nullptr};

  const std::string name_{""}; // The name in the host
  const InterfaceID ifID_{0}; // Switch interface ID

  int ifIndex_{-1}; // The ifIndex of the interface on host
  bool toDelete_{false}; // Is the interface to be deleted from system
  bool status_{false}; // Is interface UP(true)/DOWN(false)

  Interface::Addresses addrs_; // The IP addresses assigned to this intf

  /**
   * File descriptor for this interface through which packets can
   * be received from or sent to.
   */
  int fd_{-1};
  int mtu_{-1};
};

} // namespace facebook::fboss
