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

#include <folly/Conv.h>
#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/CmdConfigInterfaceSwitchportAccess.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Custom type for VLAN ID argument with validation
class VlanIdValue : public utils::BaseObjectArgType<int32_t> {
 public:
  /* implicit */ VlanIdValue(std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument("VLAN ID is required");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected single VLAN ID, got: " + folly::join(", ", v));
    }

    try {
      int32_t vlanId = folly::to<int32_t>(v[0]);
      // VLAN IDs are typically 1-4094 (0 and 4095 are reserved)
      if (vlanId < 1 || vlanId > 4094) {
        throw std::invalid_argument(
            "VLAN ID must be between 1 and 4094 inclusive, got: " +
            std::to_string(vlanId));
      }
      data_.push_back(vlanId);
    } catch (const folly::ConversionError&) {
      throw std::invalid_argument("Invalid VLAN ID: " + v[0]);
    }
  }

  int32_t getVlanId() const {
    return data_[0];
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_VLAN_ID;
};

struct CmdConfigInterfaceSwitchportAccessVlanTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterfaceSwitchportAccess;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_VLAN_ID;
  using ObjectArgType = VlanIdValue;
  using RetType = std::string;
};

class CmdConfigInterfaceSwitchportAccessVlan
    : public CmdHandler<
          CmdConfigInterfaceSwitchportAccessVlan,
          CmdConfigInterfaceSwitchportAccessVlanTraits> {
 public:
  using ObjectArgType =
      CmdConfigInterfaceSwitchportAccessVlanTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceSwitchportAccessVlanTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& vlanId);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
