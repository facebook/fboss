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
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * Custom type for queue configuration.
 *
 * Parses command line arguments in the format:
 *   <queue-id> [<attr1> <val1> [<attr2> <val2> ...]]
 *
 * For example:
 *   0 reserved-bytes 1000 weight 10 scheduling WEIGHTED_ROUND_ROBIN
 */
class QueueConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QueueConfig(std::vector<std::string> v);

  int16_t getQueueId() const {
    return queueId_;
  }

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  const std::vector<std::string>& getAqmAttributes() const {
    return aqmAttributes_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QUEUE_ID;

 private:
  int16_t queueId_{0};
  std::vector<std::pair<std::string, std::string>> attributes_;
  std::vector<std::string> aqmAttributes_;
};

struct CmdConfigQosQueuingPolicyQueueIdTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQosQueuingPolicy;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QUEUE_ID;
  using ObjectArgType = QueueConfig;
  using RetType = std::string;
};

class CmdConfigQosQueuingPolicyQueueId
    : public CmdHandler<
          CmdConfigQosQueuingPolicyQueueId,
          CmdConfigQosQueuingPolicyQueueIdTraits> {
 public:
  using ObjectArgType = CmdConfigQosQueuingPolicyQueueIdTraits::ObjectArgType;
  using RetType = CmdConfigQosQueuingPolicyQueueIdTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const QueuingPolicyName& policyName,
      const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
