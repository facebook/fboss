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

#include <folly/MacAddress.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/CmdConfigVlanStaticMac.h"

namespace facebook::fboss {

// Custom type for MAC address and port ID arguments
class MacAndPortArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MacAndPortArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v) {
    if (v.size() != 2) {
      throw std::invalid_argument(
          "Expected <mac-address> <port-name>, got " +
          std::to_string(v.size()) + " arguments");
    }

    // Validate MAC address format
    auto macAddr = folly::MacAddress::tryFromString(v[0]);
    if (!macAddr.hasValue()) {
      throw std::invalid_argument(
          "Invalid MAC address format: " + v[0] +
          ". Expected format: XX:XX:XX:XX:XX:XX");
    }
    macAddress_ = v[0];
    portName_ = v[1];
  }

  const std::string& getMacAddress() const {
    return macAddress_;
  }

  const std::string& getPortName() const {
    return portName_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_MAC_AND_PORT;

 private:
  std::string macAddress_;
  std::string portName_;
};

struct CmdConfigVlanStaticMacAddTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigVlanStaticMac;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_MAC_AND_PORT;
  using ObjectArgType = MacAndPortArg;
  using RetType = std::string;
};

class CmdConfigVlanStaticMacAdd : public CmdHandler<
                                      CmdConfigVlanStaticMacAdd,
                                      CmdConfigVlanStaticMacAddTraits> {
 public:
  using ObjectArgType = CmdConfigVlanStaticMacAddTraits::ObjectArgType;
  using RetType = CmdConfigVlanStaticMacAddTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const VlanId& vlanId,
      const ObjectArgType& macAndPort);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
