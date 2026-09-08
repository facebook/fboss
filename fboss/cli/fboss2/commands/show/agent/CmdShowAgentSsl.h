// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string_view>

#include <fboss/agent/if/gen-cpp2/fboss_types.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowAgentSslTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowAgentSslModel;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

class CmdShowAgentSsl
    : public CmdHandler<CmdShowAgentSsl, CmdShowAgentSslTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();

 private:
  static std::string sslTypeToString(const facebook::fboss::SSLType& sslType);
};

} // namespace facebook::fboss
