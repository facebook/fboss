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

#include <memory>
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class RxPacket;

/**
 * Base class for TUN interface implementations.
 * This allows for easy mocking and testing without system calls.
 */
class TunIntfBase {
 public:
  virtual ~TunIntfBase() = default;

  /**
   * Start/Stop packet forwarding on Tun interface.
   */
  virtual void start() = 0;
  virtual void stop() = 0;

  /**
   * Mark delete of the interface from the host. The interface on the host
   * will be deleted in destructor after the mark.
   */
  virtual void setDelete() = 0;

  /**
   * Change status of an interface UP/DOWN.
   * NOTE: This only changes the SW state.
   */
  virtual void setStatus(bool status) = 0;

  /**
   * Add a discovered address to the interface.
   * This function just modifies the software object.
   */
  virtual void addAddress(const folly::IPAddress& addr, uint8_t mask) = 0;

  /**
   * Set the new addresses for the interface.
   * This function just modifies the software object.
   */
  virtual void setAddresses(const Interface::Addresses& addrs) = 0;

  /**
   * Send a packet to the interface on host.
   * Unlike other methods, which are called on thread that serves the evb,
   * this function can be called from any thread.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  virtual bool sendPacketToHost(std::unique_ptr<RxPacket> pkt) = 0;

  /**
   * Set the maximum MTU on Tun interface
   */
  virtual void setMtu(int mtu) = 0;

  // Getters
  virtual int getIfIndex() const = 0;
  virtual std::string getName() const = 0;
  virtual InterfaceID getInterfaceID() const = 0;
  virtual Interface::Addresses getAddresses() const = 0;
  virtual int getMtu() const = 0;
  virtual bool getStatus() const = 0;
};

} // namespace facebook::fboss
