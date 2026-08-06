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

#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdConfigQosQueueConfigTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "queue_config_name",
        args,
        "Queue config name, or 'default' for the switch-wide default queues");
  }
  using ObjectArgType = utils::QueueConfigName;
  using RetType = std::string;
};

class CmdConfigQosQueueConfig : public CmdHandler<
                                    CmdConfigQosQueueConfig,
                                    CmdConfigQosQueueConfigTraits> {
 public:
  using ObjectArgType = CmdConfigQosQueueConfigTraits::ObjectArgType;
  using RetType = CmdConfigQosQueueConfigTraits::RetType;

  // Always throws: `queue-config <name>` is only ever a prefix of the
  // `queue-id` subcommand. Signature is fixed by CmdHandler, so it cannot be
  // declared [[noreturn]] without diverging from every other parent command.
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* name */) {
    throw std::runtime_error(
        "Incomplete command, expected: config qos queue-config "
        "<name|default> queue-id <queue-id> <attr> <value> [<attr> <value> "
        "...]");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
