// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowAgentFirmwareTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowAgentFirmwareModel;
};

class CmdShowAgentFirmware
    : public CmdHandler<CmdShowAgentFirmware, CmdShowAgentFirmwareTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(const std::vector<FirmwareInfo>& firmwareInfoList);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
