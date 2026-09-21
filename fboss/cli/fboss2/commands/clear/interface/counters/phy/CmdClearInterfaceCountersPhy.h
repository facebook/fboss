// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <vector>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearInterfaceCounters.h"

namespace facebook::fboss {

struct CmdClearInterfaceCountersPhyTraits : public WriteCommandTraits {
  using ParentCmd = CmdClearInterfaceCounters;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearInterfaceCountersPhy : public CmdHandler<
                                         CmdClearInterfaceCountersPhy,
                                         CmdClearInterfaceCountersPhyTraits> {
 public:
  using RetType = CmdClearInterfaceCountersPhyTraits::RetType;

  std::string queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs);

  void printOutput(const RetType& logMsg);

  void clearCountersPhy(const HostInfo& hostInfo, std::vector<int32_t> ports);
};

} // namespace facebook::fboss
