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
#include "fboss/cli/fboss2/commands/config/protocol/CmdConfigProtocol.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocol;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigProtocolBgp
    : public CmdHandler<CmdConfigProtocolBgp, CmdConfigProtocolBgpTraits> {
 public:
  using ObjectArgType = CmdConfigProtocolBgpTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
