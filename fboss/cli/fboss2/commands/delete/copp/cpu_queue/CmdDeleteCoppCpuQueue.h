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

#include <cstdint>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/copp/CmdDeleteCopp.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

// Argument for `delete copp cpu-queue <id>`, which removes the whole
// cpuQueues[] entry.
class CoppCpuQueueDeleteArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ CoppCpuQueueDeleteArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  int16_t getQueueId() const {
    return queueId_;
  }

 private:
  int16_t queueId_ = 0;
};

struct CmdDeleteCoppCpuQueueTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteCopp;
  using ObjectArgType = CoppCpuQueueDeleteArgs;
  using RetType = std::string;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "cpu_queue_delete", args, "<id> of the CPU queue entry to delete");
  }
};

class CmdDeleteCoppCpuQueue
    : public CmdHandler<CmdDeleteCoppCpuQueue, CmdDeleteCoppCpuQueueTraits> {
 public:
  using ObjectArgType = CmdDeleteCoppCpuQueueTraits::ObjectArgType;
  using RetType = CmdDeleteCoppCpuQueueTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
