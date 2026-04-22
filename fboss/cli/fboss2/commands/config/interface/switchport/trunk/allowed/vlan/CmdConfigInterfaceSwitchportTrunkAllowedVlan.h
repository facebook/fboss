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
#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/allowed/CmdConfigInterfaceSwitchportTrunkAllowed.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

// Use TrunkVlanAction from CmdUtils.h
using TrunkVlanAction = utils::TrunkVlanAction;

struct CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterfaceSwitchportTrunkAllowed;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_TRUNK_VLAN_ACTION;
  using ObjectArgType = TrunkVlanAction;
  using RetType = std::string;
};

class CmdConfigInterfaceSwitchportTrunkAllowedVlan
    : public CmdHandler<
          CmdConfigInterfaceSwitchportTrunkAllowedVlan,
          CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits> {
 public:
  using ObjectArgType =
      CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& vlanAction);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
