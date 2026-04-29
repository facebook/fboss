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
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `config routing <attr> <value>`.
// Validates that v[0] is one of the valid routing attribute names and v[1] is
// a non-negative int32.
class RoutingConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ RoutingConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getAttribute() const {
    return attribute_;
  }

  int32_t getValue() const {
    return value_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ROUTING_CONFIG;

 private:
  std::string attribute_;
  int32_t value_ = 0;
};

struct CmdConfigRoutingTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ROUTING_CONFIG;
  using ObjectArgType = RoutingConfigArgs;
  using RetType = std::string;
};

class CmdConfigRouting
    : public CmdHandler<CmdConfigRouting, CmdConfigRoutingTraits> {
 public:
  using ObjectArgType = CmdConfigRoutingTraits::ObjectArgType;
  using RetType = CmdConfigRoutingTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
