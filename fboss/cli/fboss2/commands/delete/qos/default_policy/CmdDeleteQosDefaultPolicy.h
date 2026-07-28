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

#include <string>
#include <variant>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/qos/CmdDeleteQos.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdDeleteQosDefaultPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteQos;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdDeleteQosDefaultPolicy : public CmdHandler<
                                      CmdDeleteQosDefaultPolicy,
                                      CmdDeleteQosDefaultPolicyTraits> {
 public:
  using ObjectArgType = CmdDeleteQosDefaultPolicyTraits::ObjectArgType;
  using RetType = CmdDeleteQosDefaultPolicyTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
