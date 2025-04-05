// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "CmdShowAgentFirmware.h"

namespace facebook::fboss {

CmdShowAgentFirmwareTraits::RetType CmdShowAgentFirmware::queryClient(
    const HostInfo& hostInfo) {
  auto client = utils::createAgentClient(hostInfo);

  CmdShowAgentFirmwareTraits::RetType model{};
  try {
    FirmwareInfo firmwareInfo;
    client->sync_getFirmwareInfo(firmwareInfo);
    model.version() = firmwareInfo.version().value();
    model.opStatus() =
        apache::thrift::util::enumNameSafe(firmwareInfo.opStatus().value());
    model.funcStatus() =
        apache::thrift::util::enumNameSafe(firmwareInfo.funcStatus().value());
  } catch (const std::exception& e) {
    // TODO remove when Agent with sync_getFirmwareInfo is rolled out
    // everywhere.
    ;
  }

  return model;
}

void CmdShowAgentFirmware::printOutput(
    const CmdShowAgentFirmwareTraits::RetType& model,
    std::ostream& out) {
  out << "Version: " << model.version().value() << std::endl;
  out << "Operational Status: " << model.opStatus().value() << std::endl;
  out << "Functional Status: " << model.funcStatus().value() << std::endl;
}

} // namespace facebook::fboss
