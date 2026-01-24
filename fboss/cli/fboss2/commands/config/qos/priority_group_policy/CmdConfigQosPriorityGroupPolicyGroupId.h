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
#include "fboss/cli/fboss2/commands/config/qos/priority_group_policy/CmdConfigQosPriorityGroupPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * Custom type for priority group configuration.
 *
 * Parses command line arguments in the format:
 *   <group-id> [<attr1> <val1> [<attr2> <val2> ...]]
 *
 * For example:
 *   2 min-limit-bytes 42 headroom-limit-bytes 2048
 */
class PriorityGroupConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ PriorityGroupConfig(std::vector<std::string> v);

  int16_t getGroupId() const {
    return groupId_;
  }

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PRIORITY_GROUP_ID;

 private:
  int16_t groupId_{0};
  std::vector<std::pair<std::string, std::string>> attributes_;
};

struct CmdConfigQosPriorityGroupPolicyGroupIdTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigQosPriorityGroupPolicy;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PRIORITY_GROUP_ID;
  using ObjectArgType = PriorityGroupConfig;
  using RetType = std::string;
};

class CmdConfigQosPriorityGroupPolicyGroupId
    : public CmdHandler<
          CmdConfigQosPriorityGroupPolicyGroupId,
          CmdConfigQosPriorityGroupPolicyGroupIdTraits> {
 public:
  using ObjectArgType =
      CmdConfigQosPriorityGroupPolicyGroupIdTraits::ObjectArgType;
  using RetType = CmdConfigQosPriorityGroupPolicyGroupIdTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const PriorityGroupPolicyName& policyName,
      const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
