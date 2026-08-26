/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/json/dynamic.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

struct CmdShowBgpPolicyTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = std::vector<std::string>;
  using RetType = folly::dynamic;
};

class CmdShowBgpPolicy
    : public CmdHandler<CmdShowBgpPolicy, CmdShowBgpPolicyTraits> {
 public:
  using RetType = CmdShowBgpPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& policyNames);
  void printOutput(RetType& policyConfig, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
