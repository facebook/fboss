/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

inline const std::string kBounceInterfaceYesFlag = "-y,--yes";
inline const std::string kBounceInterfaceCommandName = "bounce_interface";

struct CmdBounceInterfaceTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
  std::vector<utils::LocalOption> LocalOptions = {
      {kBounceInterfaceYesFlag,
       "Skip the Safer Human Touch confirmation prompt. Use only when you "
       "have read the warning and accept the risk.",
       std::nullopt,
       /*isFlag=*/true},
  };
};

class CmdBounceInterface
    : public CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits> {
 public:
  using RetType = CmdBounceInterfaceTraits::RetType;
  static constexpr int SECONDS_TO_SLEEP = 5;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs);

  void printActionText(int32_t actionCode);

  std::string getPortStatus(
      int32_t port,
      const std::vector<int32_t>& portsToBounce,
      apache::thrift::Client<FbossCtrl>& client);

  void changeInterfaceStatus(
      int32_t portId,
      bool enabled,
      apache::thrift::Client<FbossCtrl>& client);

  void printOutput(const RetType& bounceResult);
};
} // namespace facebook::fboss
