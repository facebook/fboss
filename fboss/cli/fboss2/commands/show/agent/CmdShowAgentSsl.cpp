// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "CmdShowAgentSsl.h"

#include <iostream>
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

CmdShowAgentSsl::RetType CmdShowAgentSsl::queryClient(
    const HostInfo& hostInfo) {
  auto client = utils::createAgentClient(hostInfo);
  SSLType sslType = client->sync_getSSLPolicy();

  RetType model{};
  model.AgentSslStatus() = sslTypeToString(sslType);
  return model;
}

void CmdShowAgentSsl::printOutput(const RetType& model, std::ostream& out) {
  out << "Secure Thrift server SSL config: " << model.AgentSslStatus().value()
      << std::endl;
}

std::string CmdShowAgentSsl::sslTypeToString(
    const facebook::fboss::SSLType& sslType) {
  switch (sslType) {
    case SSLType::DISABLED:
      return "DISABLED (allow plaintext clients only)";
    case SSLType::PERMITTED:
      return "PERMITTED (allow plaintext & encrypted clients)";
    case SSLType::REQUIRED:
      return "REQUIRED (allow encrypted clients only)";
    default:
      return "UNKNOWN - unexpected value of SSLType";
  }
}

} // namespace facebook::fboss
