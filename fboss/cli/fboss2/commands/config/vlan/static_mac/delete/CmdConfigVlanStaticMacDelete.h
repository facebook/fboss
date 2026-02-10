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

// Custom type for MAC address argument only (no port needed for delete)
class MacAddressArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MacAddressArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v) {
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected <mac-address>, got " + std::to_string(v.size()) +
          " arguments");
    }

    // Validate MAC address format
    auto macAddr = folly::MacAddress::tryFromString(v[0]);
    if (!macAddr.hasValue()) {
      throw std::invalid_argument(
          "Invalid MAC address format: " + v[0] +
          ". Expected format: XX:XX:XX:XX:XX:XX");
    }
    macAddress_ = v[0];
  }

  const std::string& getMacAddress() const {
    return macAddress_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;

 private:
  std::string macAddress_;
};

struct CmdConfigVlanStaticMacDeleteTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigVlanStaticMac;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = MacAddressArg;
  using RetType = std::string;
};

class CmdConfigVlanStaticMacDelete : public CmdHandler<
                                         CmdConfigVlanStaticMacDelete,
                                         CmdConfigVlanStaticMacDeleteTraits> {
 public:
  using ObjectArgType = CmdConfigVlanStaticMacDeleteTraits::ObjectArgType;
  using RetType = CmdConfigVlanStaticMacDeleteTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const VlanId& vlanId,
      const ObjectArgType& macAddress);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
