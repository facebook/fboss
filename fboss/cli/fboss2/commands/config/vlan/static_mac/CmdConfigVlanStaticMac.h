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
#include "fboss/cli/fboss2/commands/config/vlan/CmdConfigVlan.h"

namespace facebook::fboss {

struct CmdConfigVlanStaticMacTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigVlan;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigVlanStaticMac
    : public CmdHandler<CmdConfigVlanStaticMac, CmdConfigVlanStaticMacTraits> {
 public:
  using ObjectArgType = CmdConfigVlanStaticMacTraits::ObjectArgType;
  using RetType = CmdConfigVlanStaticMacTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const VlanId& /* vlanId */) {
    throw std::runtime_error(
        "Incomplete command, please use 'add' or 'delete' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
