// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>

namespace facebook::fboss {

std::map<std::string, int32_t> getQueriedPortIds(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& entries,
    const std::vector<std::string>& queriedPorts);

struct CmdSetPortStateTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_STATE;
  using ParentCmd = CmdSetPort;
  using ObjectArgType = utils::PortState;
  using RetType = std::string;
};

class CmdSetPortState
    : public CmdHandler<CmdSetPortState, CmdSetPortStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts,
      const utils::PortState& state);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
