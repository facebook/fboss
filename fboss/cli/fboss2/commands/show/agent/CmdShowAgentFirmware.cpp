// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "CmdShowAgentFirmware.h"

namespace facebook::fboss {

using RetType = CmdShowAgentFirmwareTraits::RetType;

CmdShowAgentFirmwareTraits::RetType CmdShowAgentFirmware::queryClient(
    const HostInfo& hostInfo) {
  auto client = utils::createAgentClient(hostInfo);

  CmdShowAgentFirmwareTraits::RetType model{};
  std::vector<FirmwareInfo> entries;

  try {
    auto hwAgentQueryFn =
        [&entries](
            apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
          std::vector<FirmwareInfo> firmwareInfoList;
          client.sync_getAllHwFirmwareInfo(firmwareInfoList);
          entries.insert(
              entries.end(), firmwareInfoList.begin(), firmwareInfoList.end());
        };
    utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);
  } catch (const std::exception&) {
    // TODO remove when Agent with sync_getAllHwFirmwareInfo is rolled out
    // everywhere.
  }

  return createModel(entries);
}

RetType CmdShowAgentFirmware::createModel(
    const std::vector<FirmwareInfo>& firmwareInfoList) {
  RetType model;

  for (const auto& firmwareInfo : firmwareInfoList) {
    cli::AgentFirmwareEntry entry;

    entry.version() = firmwareInfo.version().value();
    entry.opStatus() =
        apache::thrift::util::enumNameSafe(firmwareInfo.opStatus().value());
    entry.funcStatus() =
        apache::thrift::util::enumNameSafe(firmwareInfo.funcStatus().value());

    model.firmwareEntries()->push_back(entry);
  }

  return model;
}

void CmdShowAgentFirmware::printOutput(
    const CmdShowAgentFirmwareTraits::RetType& model,
    std::ostream& out) {
  for (const auto& entry : *model.firmwareEntries()) {
    out << "Version: " << entry.version().value() << std::endl;
    out << "Operational Status: " << entry.opStatus().value() << std::endl;
    out << "Functional Status: " << entry.funcStatus().value() << std::endl;
  }
}

} // namespace facebook::fboss
