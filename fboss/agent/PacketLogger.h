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

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/StateDelta.h"

#include <memory>
#include <vector>

namespace facebook::fboss {
class SwSwitch;

class PacketLoggerBase {
 public:
  virtual ~PacketLoggerBase() {}

  virtual void log(
      std::string pktType, // ARP, NDP, or LLP
      std::string pktDir, // RX or TX
      std::optional<VlanID> vlanID,
      std::string srcMac,
      std::string senderIP,
      std::string targetIP) = 0;
};

class PacketLogger : public PacketLoggerBase {
 public:
  explicit PacketLogger(SwSwitch* sw);
  PacketLogger() {}

  void log(
      std::string pktType,
      std::string pktDir,
      std::optional<VlanID> vlanID,
      std::string srcMac,
      std::string senderIP,
      std::string targetIP) override;
};

} // namespace facebook::fboss
