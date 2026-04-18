// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/reboot_cause/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/platform/reboot_cause_service/if/gen-cpp2/reboot_cause_service_types.h"

namespace facebook::fboss {

struct CmdShowRebootCauseTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowRebootCauseModel;
};

class CmdShowRebootCause
    : public CmdHandler<CmdShowRebootCause, CmdShowRebootCauseTraits> {
 public:
  using ObjectArgType = CmdShowRebootCauseTraits::ObjectArgType;
  using RetType = CmdShowRebootCauseTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(
      const platform::reboot_cause_service::RebootCauseResult& result);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
