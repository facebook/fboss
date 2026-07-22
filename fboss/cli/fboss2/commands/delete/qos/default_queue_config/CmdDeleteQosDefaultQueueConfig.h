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

#include <fmt/format.h>
#include <folly/Conv.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/qos/CmdDeleteQos.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

class DeleteQueueId : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ DeleteQueueId(std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument("queue-id is required");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          fmt::format("Expected exactly one queue-id, got {}", v.size()));
    }
    auto parsed = folly::tryTo<int16_t>(v[0]);
    if (!parsed.hasValue()) {
      throw std::invalid_argument(
          fmt::format("Invalid queue-id: '{}'. Must be an integer", v[0]));
    }
    // Shared upper bound (utils::kMaxQueueId): the same limit the config
    // command's queue-id parser enforces, kept in one place so they can't
    // drift.
    if (parsed.value() < 0 || parsed.value() > utils::kMaxQueueId) {
      throw std::invalid_argument(
          fmt::format(
              "queue-id must be between 0 and {}, got: {}",
              utils::kMaxQueueId,
              v[0]));
    }
    queueId_ = parsed.value();
    data_ = std::move(v);
  }

  int16_t getQueueId() const {
    return queueId_;
  }

 private:
  int16_t queueId_{0};
};

struct CmdDeleteQosDefaultQueueConfigTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteQos;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "queue_id",
           args,
           "ID of the queue entry to remove from defaultPortQueues")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = DeleteQueueId;
  using RetType = std::string;
};

class CmdDeleteQosDefaultQueueConfig
    : public CmdHandler<
          CmdDeleteQosDefaultQueueConfig,
          CmdDeleteQosDefaultQueueConfigTraits> {
 public:
  using ObjectArgType = CmdDeleteQosDefaultQueueConfigTraits::ObjectArgType;
  using RetType = CmdDeleteQosDefaultQueueConfigTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& queueId);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
