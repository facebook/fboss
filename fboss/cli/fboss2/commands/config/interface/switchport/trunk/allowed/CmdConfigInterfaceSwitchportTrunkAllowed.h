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
#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/CmdConfigInterfaceSwitchportTrunk.h"

namespace facebook::fboss {

struct CmdConfigInterfaceSwitchportTrunkAllowedTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterfaceSwitchportTrunk;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigInterfaceSwitchportTrunkAllowed
    : public CmdHandler<
          CmdConfigInterfaceSwitchportTrunkAllowed,
          CmdConfigInterfaceSwitchportTrunkAllowedTraits> {
 public:
  using RetType = CmdConfigInterfaceSwitchportTrunkAllowedTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces);

  void printOutput(const RetType& model);
};

} // namespace facebook::fboss
