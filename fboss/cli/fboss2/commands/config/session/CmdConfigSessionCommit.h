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
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigSessionCommitTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigSessionCommit
    : public CmdHandler<CmdConfigSessionCommit, CmdConfigSessionCommitTraits> {
 public:
  using ObjectArgType = CmdConfigSessionCommitTraits::ObjectArgType;
  using RetType = CmdConfigSessionCommitTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
