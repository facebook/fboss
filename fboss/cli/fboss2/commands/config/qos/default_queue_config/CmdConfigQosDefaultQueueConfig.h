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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicyQueueId.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdConfigQosDefaultQueueConfigTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "queue_config",
        args,
        "Queue ID followed by key-value pairs: <queue-id> <attr> <value> "
        "[<attr> <value> ...] where <attr> is one of: reserved-bytes, "
        "shared-bytes, weight, scaling-factor, scheduling, stream-type, "
        "buffer-pool-name, active-queue-management");
  }
  using ObjectArgType = QueueConfig;
  using RetType = std::string;
};

class CmdConfigQosDefaultQueueConfig
    : public CmdHandler<
          CmdConfigQosDefaultQueueConfig,
          CmdConfigQosDefaultQueueConfigTraits> {
 public:
  using ObjectArgType = CmdConfigQosDefaultQueueConfigTraits::ObjectArgType;
  using RetType = CmdConfigQosDefaultQueueConfigTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
