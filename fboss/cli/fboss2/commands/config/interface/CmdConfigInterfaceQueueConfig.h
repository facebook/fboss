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

#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

struct CmdConfigInterfaceQueueConfigTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("queue_config_name", args, "Queue config name");
  }
  using ObjectArgType = utils::QueueConfigName;
  using RetType = std::string;
};

class CmdConfigInterfaceQueueConfig : public CmdHandler<
                                          CmdConfigInterfaceQueueConfig,
                                          CmdConfigInterfaceQueueConfigTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceQueueConfigTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceQueueConfigTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& name);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
