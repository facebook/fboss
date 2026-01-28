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
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

// Use VlanIdValue from CmdUtils.h
using VlanId = utils::VlanIdValue;

struct CmdConfigVlanTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_VLAN_ID;
  using ObjectArgType = VlanId;
  using RetType = std::string;
};

class CmdConfigVlan : public CmdHandler<CmdConfigVlan, CmdConfigVlanTraits> {
 public:
  using ObjectArgType = CmdConfigVlanTraits::ObjectArgType;
  using RetType = CmdConfigVlanTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* vlanId */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
