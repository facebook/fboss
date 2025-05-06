// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/commands/show/platformshowtech/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/commands/show/platformshowtech/Showtech.h"

namespace facebook::fboss {

struct CmdShowPlatformShowtechTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowPlatformShowtechModel;
  std::vector<utils::LocalOption> LocalOptions = {
    {"-q", "'true' -> Run command in quite mode\n"}};
};

class CmdShowPlatformShowtech : public CmdHandler<CmdShowPlatformShowtech,
                                       CmdShowPlatformShowtechTraits> {
 public:
  using ObjectArgType = CmdShowPlatformShowtechTraits::ObjectArgType;
  using RetType = CmdShowPlatformShowtechTraits::RetType;
  using Showtech = show::platformshowtech::Showtech;
  using GenericShowtech = show::platformshowtech::GenericShowtech;

  std::string version = "1.4";

  RetType queryClient(const HostInfo& hostInfo) {
    bool quite = false;
    auto quite_str =
      CmdLocalOptions::getInstance()->getLocalOption("show_tech", "-q");

    if (!quite_str.empty() && quite_str == "true") {
      quite = true;
    }

    return createModel(quite);
  }

  RetType createModel(bool quite) {
    RetType model;
    model.version() = version;
    model.quite() = quite;

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    bool verbose = true;
    std::unique_ptr<Showtech> platformShowtech;

    if (model.get_quite()) {
        verbose = false;
    }

    platformShowtech = get_platform_showtech(verbose);
    platformShowtech->printShowtech();
  }

  std::unique_ptr<Showtech> get_platform_showtech(bool verbose) {
    return std::make_unique<GenericShowtech>(verbose);
  }
};

} // namespace facebook::fboss
