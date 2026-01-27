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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/CmdConfigInterfaceSwitchportAccess.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Use VlanIdValue from CmdUtils.h
using VlanIdValue = utils::VlanIdValue;

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
