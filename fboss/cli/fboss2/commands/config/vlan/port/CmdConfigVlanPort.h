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

struct CmdConfigVlanPortTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigVlan;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = std::string;
};

class CmdConfigVlanPort
    : public CmdHandler<CmdConfigVlanPort, CmdConfigVlanPortTraits> {
 public:
  using ObjectArgType = CmdConfigVlanPortTraits::ObjectArgType;
  using RetType = CmdConfigVlanPortTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const VlanId& /* vlanId */,
      const ObjectArgType& /* portList */) {
    throw std::runtime_error(
        "Incomplete command, please use 'taggingMode' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
