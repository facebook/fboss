// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/sdk/reg_dump/CmdSetSdkRegDump.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>

namespace facebook::fboss {

CmdSetSdkRegDump::RetType CmdSetSdkRegDump::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& state) {
  if (state.data().empty()) {
    throw std::runtime_error(
        "Incomplete command, expecting 'reg-dump <enable|disable>'");
  }
  if (state.data().size() != 1) {
    throw std::runtime_error(
        folly::to<std::string>(
            "Unexpected state '",
            folly::join(" ", state.data()),
            "', expecting 'enable|disable'"));
  }
  const auto stateStr = boost::to_upper_copy(state.data()[0]);
  if (stateStr != "ENABLE" && stateStr != "DISABLE") {
    throw std::runtime_error(
        folly::to<std::string>(
            "Unexpected state '",
            state.data()[0],
            "', expecting 'enable|disable'"));
  }
  const bool enabled = (stateStr == "ENABLE");

  auto client =
      utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
          hostInfo);
  // Let the agent's FbossError (e.g. "not supported on this device") propagate
  // so the CLI exits non-zero and scripts can detect the failure.
  client->sync_setSdkRegDumpEnabled(enabled);

  const std::string host =
      hostInfo.isLocalHost() ? "" : fmt::format(" on {}", hostInfo.getName());
  return fmt::format(
      "{} SDK register/state dump{}", enabled ? "Enabled" : "Disabled", host);
}

void CmdSetSdkRegDump::printOutput(const RetType& model, std::ostream& out) {
  out << model << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdSetSdkRegDump, CmdSetSdkRegDumpTraits>::run();

} // namespace facebook::fboss
