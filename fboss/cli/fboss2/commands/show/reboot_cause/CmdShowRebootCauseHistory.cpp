// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/reboot_cause/CmdShowRebootCauseHistory.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/platform/reboot_cause_service/if/gen-cpp2/RebootCauseService.h"

#include <fmt/format.h>

namespace facebook::fboss {

CmdShowRebootCauseHistory::RetType CmdShowRebootCauseHistory::queryClient(
    const HostInfo& hostInfo) {
  auto port =
      CmdGlobalOptions::getInstance()->getRebootCauseServiceThriftPort();
  auto client = utils::createPlaintextClient<apache::thrift::Client<
      platform::reboot_cause_service::RebootCauseService>>(hostInfo, port);
  std::vector<platform::reboot_cause_service::RebootCauseResult> results;
  client->sync_getRebootCauseHistory(results);
  return createModel(results);
}

CmdShowRebootCauseHistory::RetType CmdShowRebootCauseHistory::createModel(
    const std::vector<platform::reboot_cause_service::RebootCauseResult>&
        results) {
  RetType model;
  for (const auto& result : results) {
    cli::ShowRebootCauseModel entry;
    entry.investigationDate() = result.date().value();
    entry.causeDescription() = result.determinedCause()->description().value();
    entry.causeDate() = result.determinedCause()->date().value();
    entry.causeProvider() = result.determinedCauseProvider().value();
    model.entries()->push_back(entry);
  }
  return model;
}

void CmdShowRebootCauseHistory::printOutput(
    const RetType& model,
    std::ostream& out) {
  const auto& entries = model.entries().value();
  if (entries.empty()) {
    out << "No reboot cause history found." << std::endl;
    return;
  }
  auto truncate = [](const std::string& s, size_t maxLen) -> std::string {
    if (s.size() <= maxLen) {
      return s;
    }
    return s.substr(0, maxLen - 3) + "...";
  };
  constexpr auto fmtString = "{:<24}{:<32}{:<24}{}\n";
  out << fmt::format(
      fmtString, "Collection Time", "Description", "Provider", "Cause Time");
  out << std::string(88, '-') << std::endl;
  for (const auto& entry : entries) {
    out << fmt::format(
        fmtString,
        entry.investigationDate().value(),
        truncate(entry.causeDescription().value(), 32),
        entry.causeProvider().value(),
        entry.causeDate().value());
  }
}

// Explicit template instantiation
template void
CmdHandler<CmdShowRebootCauseHistory, CmdShowRebootCauseHistoryTraits>::run();

} // namespace facebook::fboss
