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

namespace facebook::fboss {

struct CmdConfigSwitchTraits : public WriteCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdConfigSwitch
    : public CmdHandler<CmdConfigSwitch, CmdConfigSwitchTraits> {
 public:
  using ObjectArgType = CmdConfigSwitchTraits::ObjectArgType;
  using RetType = CmdConfigSwitchTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
