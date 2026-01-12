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

struct CmdConfigSessionDiffTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_REVISION_LIST;
  using ObjectArgType = utils::RevisionList;
  using RetType = std::string;
};

class CmdConfigSessionDiff
    : public CmdHandler<CmdConfigSessionDiff, CmdConfigSessionDiffTraits> {
 public:
  using ObjectArgType = CmdConfigSessionDiffTraits::ObjectArgType;
  using RetType = CmdConfigSessionDiffTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::RevisionList& revisions);

  void printOutput(const RetType& diffOutput);
};

} // namespace facebook::fboss
