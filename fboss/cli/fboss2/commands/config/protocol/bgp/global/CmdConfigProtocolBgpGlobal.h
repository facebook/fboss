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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/CmdConfigProtocolBgp.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpGlobalTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgp;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigProtocolBgpGlobal : public CmdHandler<
                                       CmdConfigProtocolBgpGlobal,
                                       CmdConfigProtocolBgpGlobalTraits> {
 public:
  using ObjectArgType = CmdConfigProtocolBgpGlobalTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpGlobalTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
