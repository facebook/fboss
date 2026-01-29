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
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * Custom type for QoS policy name.
 */
class QosPolicyName : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QosPolicyName(std::vector<std::string> v)
      : BaseObjectArgType(std::move(v)) {}

  std::string getName() const {
    return data_.empty() ? "" : data_[0];
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QOS_POLICY_NAME;
};

struct CmdConfigQosPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QOS_POLICY_NAME;
  using ObjectArgType = QosPolicyName;
  using RetType = std::string;
};

class CmdConfigQosPolicy
    : public CmdHandler<CmdConfigQosPolicy, CmdConfigQosPolicyTraits> {
 public:
  using ObjectArgType = CmdConfigQosPolicyTraits::ObjectArgType;
  using RetType = CmdConfigQosPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* policyName */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands: map");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
