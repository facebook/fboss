// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fmt/core.h>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h" // NOLINT(misc-include-cleaner)
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h" // NOLINT(misc-include-cleaner)
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;

struct CmdShowBgpStreamSubscriberTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PEERID_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::vector<std::string>;
};

class CmdShowBgpStreamSubscriber : public CmdHandler<
                                       CmdShowBgpStreamSubscriber,
                                       CmdShowBgpStreamSubscriberTraits> {
 public:
  using RetType = CmdShowBgpStreamSubscriberTraits::RetType;
  using ObjectArgType = CmdShowBgpStreamSubscriberTraits::ObjectArgType;
  RetType queryClient(const HostInfo&, const ObjectArgType& peerIds) {
    if (peerIds.empty()) {
      std::cout
          << "No subscriber id entered. Usage: fboss2 show bgp stream subscriber <subscriber id> pre-policy/post-policy"
          << std::endl;
    }
    return peerIds;
  }

  void printOutput(RetType& peerIds) {
    if (!peerIds.empty()) {
      std::cout
          << fmt::format(
                 "Missing policy argument: fboss2 show bgp stream subscriber {} pre-policy/post-policy",
                 peerIds[0])
          << std::endl;
    }
  }
};

} // namespace facebook::fboss
