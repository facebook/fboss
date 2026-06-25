// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/reboot_cause/CmdShowRebootCause.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/platform/reboot_cause_service/if/gen-cpp2/RebootCauseService.h"

namespace facebook::fboss {

CmdShowRebootCause::RetType CmdShowRebootCause::queryClient(
    const HostInfo& hostInfo) {
  auto port =
      CmdGlobalOptions::getInstance()->getRebootCauseServiceThriftPort();
  auto client = utils::createPlaintextClient<apache::thrift::Client<
      platform::reboot_cause_service::RebootCauseService>>(hostInfo, port);
  platform::reboot_cause_service::RebootCauseResult result;
  client->sync_getLastRebootCause(result);
  return createModel(result);
}

CmdShowRebootCause::RetType CmdShowRebootCause::createModel(
    const platform::reboot_cause_service::RebootCauseResult& result) {
  RetType model;
  model.investigationDate() = result.date().value();
  model.causeDescription() = result.determinedCause()->description().value();
  model.causeDate() = result.determinedCause()->date().value();
  model.causeProvider() = result.determinedCauseProvider().value();
  return model;
}

void CmdShowRebootCause::printOutput(const RetType& model, std::ostream& out) {
  out << model.causeDescription().value()
      << " [Provider: " << model.causeProvider().value()
      << ", Time: " << model.causeDate().value() << "]" << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdShowRebootCause, CmdShowRebootCauseTraits>::run();

} // namespace facebook::fboss
