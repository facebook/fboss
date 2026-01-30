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

#include <folly/String.h>
#include <re2/re2.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Custom type for queuing policy name argument with validation
class QueuingPolicyName : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QueuingPolicyName(std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument("Queuing policy name is required");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected single queuing policy name, got: " + folly::join(", ", v));
    }
    const auto& name = v[0];
    // Valid policy name: starts with letter, alphanumeric + underscore/hyphen,
    // 1-64 chars
    static const re2::RE2 kValidPolicyNamePattern(
        "^[a-zA-Z][a-zA-Z0-9_-]{0,63}$");
    if (!re2::RE2::FullMatch(name, kValidPolicyNamePattern)) {
      throw std::invalid_argument(
          "Invalid queuing policy name: '" + name +
          "'. Name must start with a letter, contain only alphanumeric "
          "characters, underscores, or hyphens, and be 1-64 characters long.");
    }
    data_.push_back(name);
  }

  const std::string& getName() const {
    return data_[0];
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QUEUING_POLICY_NAME;
};

struct CmdConfigQosQueuingPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QUEUING_POLICY_NAME;
  using ObjectArgType = QueuingPolicyName;
  using RetType = std::string;
};

class CmdConfigQosQueuingPolicy : public CmdHandler<
                                      CmdConfigQosQueuingPolicy,
                                      CmdConfigQosQueuingPolicyTraits> {
 public:
  using ObjectArgType = CmdConfigQosQueuingPolicyTraits::ObjectArgType;
  using RetType = CmdConfigQosQueuingPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* policyName */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands: "
        "reserved-bytes, scaling-factor, scheduling, weight, "
        "shared-bytes, buffer-pool-name");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
