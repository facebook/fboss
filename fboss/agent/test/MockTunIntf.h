/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @file MockTunIntf.h
 * @brief Mock implementation of TUN interface for unit testing
 * @author ashutosh grewal
 *
 * This file provides a mock implementation of the TunIntfBase that enables
 * comprehensive unit testing of TUN interface functionality without requiring
 * actual system calls.
 */

#pragma once

#include <gmock/gmock.h>
#include "fboss/agent/TunIntfBase.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class MockTunIntf : public TunIntfBase {
 public:
  MockTunIntf(
      const InterfaceID& ifID,
      const std::string& name,
      int ifIndex,
      int mtu = 1500);

  ~MockTunIntf() override = default;

  // Mock methods not called in deleteProbedAddressesAndRules
  MOCK_METHOD0(start, void());
  MOCK_METHOD0(stop, void());
  MOCK_METHOD0(setDelete, void());
  MOCK_METHOD1(setStatus, void(bool status));
  MOCK_METHOD2(addAddress, void(const folly::IPAddress& addr, uint8_t mask));
  MOCK_METHOD1(setAddresses, void(const Interface::Addresses& addrs));
  MOCK_METHOD1(sendPacketToHost, bool(std::unique_ptr<RxPacket> pkt));
  MOCK_METHOD1(setMtu, void(int mtu));
  MOCK_CONST_METHOD0(getInterfaceID, InterfaceID());
  MOCK_CONST_METHOD0(getMtu, int());
  MOCK_CONST_METHOD0(getStatus, bool());

  // Real implementations for methods called in deleteProbedAddressesAndRules
  int getIfIndex() const override {
    return ifIndex_;
  }

  std::string getName() const override {
    return name_;
  }

  Interface::Addresses getAddresses() const override {
    return addrs_;
  }

  // Helper methods for tests to set state
  void setTestAddresses(const Interface::Addresses& addrs) {
    addrs_ = addrs;
  }

 private:
  InterfaceID ifID_;
  std::string name_;
  int ifIndex_;
  Interface::Addresses addrs_;
};

} // namespace facebook::fboss
