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
#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"

#include <string>
#include <variant>

namespace facebook::fboss {

struct CmdClearInterfaceCountersTraits : public WriteCommandTraits {
  using ParentCmd = CmdClearInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearInterfaceCounters : public CmdHandler<
                                      CmdClearInterfaceCounters,
                                      CmdClearInterfaceCountersTraits> {
 public:
  using RetType = CmdClearInterfaceCountersTraits::RetType;

  std::string queryClient(
      const HostInfo& hostInfo,
      std::vector<std::string> queriedIfs);

  void printOutput(const RetType& logMsg);
};
} // namespace facebook::fboss
