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

#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <fboss/fsdb/if/gen-cpp2/fsdb_common_types.h>
#include "fboss/fsdb/if/FsdbModel.h"

#include <thrift/lib/cpp2/gen/module_types_h.h>
#include "fboss/cli/fboss2/CmdHandler.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"

namespace facebook::fboss {

struct CmdShowFsdbSubscriberTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_CLIENT_ID;
  using ObjectArgType = utils::FsdbClientId;
  using RetType = facebook::fboss::fsdb::SubscriberIdToOperSubscriberInfos;
};

class CmdShowFsdbSubscribers
    : public CmdHandler<CmdShowFsdbSubscribers, CmdShowFsdbSubscriberTraits> {
 public:
  using ObjectArgType = CmdShowFsdbSubscriberTraits::ObjectArgType;
  using RetType = CmdShowFsdbSubscriberTraits::RetType;
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& fsdbClientid);
  void printOutput(const RetType& result, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
