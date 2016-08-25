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
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>

namespace facebook { namespace fboss {

class SwSwitch;
class RxPacket;

class TunIntf : private folly::EventHandler {
 public:
  /**
   * Creates a TunIntf object of already existing linux interface.
   */
  TunIntf(
      SwSwitch *sw,
      folly::EventBase *evb,
      InterfaceID ifID,
      int ifIndex /* linux */,
      int mtu);

  /**
   * This version of constructor creates a Tun interface in Linux as well.
   */
  TunIntf(
      SwSwitch *sw,
      folly::EventBase *evb,
      InterfaceID ifID,   // Switch interface ID
      const Interface::Addresses& addrs,
      int mtu);

  ~TunIntf() override;

  /**
   * Utility functions for InterfaceID <-> ifName (on host)
   */
  static std::string createTunIntfName(InterfaceID ifID);
  static bool isTunIntfName(const std::string& ifName);
  static InterfaceID getIDFromTunIntfName(const std::string& ifName);

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

  SwSwitch *sw_{nullptr};

  const std::string name_{""};  // The name in the host
  const InterfaceID ifID_{0};   // Switch interface ID

  int ifIndex_{-1};             // The ifIndex of the interface on host
  bool toDelete_{false};        // Is the interface to be deleted from system

  Interface::Addresses addrs_;  // The IP addresses assigned to this intf

  /**
   * File descriptor for this interface through which packets can
   * be received from or sent to.
   */
  int fd_{-1};
  int mtu_{-1};
};

}}  // nanesoace facebook::fboss
