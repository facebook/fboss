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
 * @file MockTunIntf.cpp
 * @brief Implementation of mock TUN interface for unit testing
 * @author ashutosh grewal
 *
 * This file implements the MockTunIntf class which provides a mock
 * implementation of the TunIntfBase. It enables unit testing of TUN
 * interface functionality without requiring actual system calls or kernel
 * interactions.
 */

#include "fboss/agent/test/MockTunIntf.h"

namespace facebook::fboss {

MockTunIntf::MockTunIntf(
    const InterfaceID& ifID,
    const std::string& name,
    int ifIndex,
    int mtu)
    : ifID_(ifID), name_(name), ifIndex_(ifIndex) {
  // No system calls made in this constructor
}

} // namespace facebook::fboss
