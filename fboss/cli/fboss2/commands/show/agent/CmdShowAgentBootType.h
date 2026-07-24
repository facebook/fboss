// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

struct AgentBootType {
  std::string agentName;
  std::optional<int32_t> switchIndex;
  BootType bootType;
};

struct CmdShowAgentBootTypeTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowAgentBootTypeModel;
};

class CmdShowAgentBootType
    : public CmdHandler<CmdShowAgentBootType, CmdShowAgentBootTypeTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(const std::vector<AgentBootType>& bootTypes) const;
  void printOutput(const RetType& model, std::ostream& out = std::cout);

 private:
  static std::string bootTypeToString(BootType bootType);
};

} // namespace facebook::fboss
