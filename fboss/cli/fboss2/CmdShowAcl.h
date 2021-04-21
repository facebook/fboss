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

namespace facebook::fboss {

struct CmdShowAclTraits {
  using ClientType = facebook::fboss::FbossCtrlAsyncClient;
  using RetType = std::vector<facebook::fboss::AclEntryThrift>;
};

class CmdShowAcl : public CmdHandler<CmdShowAcl, CmdShowAclTraits> {
 public:
  using ClientType = CmdShowAclTraits::ClientType;
  using RetType = CmdShowAclTraits::RetType;

  RetType queryClient(
      const std::unique_ptr<ClientType>& client) {
    RetType retVal;

    client->sync_getAclTable(retVal);
    return retVal;
  }

  void printOutput(const RetType& aclTable) {
    for (auto const& aclEntry : aclTable) {
      std::cout << aclEntry.get_name() << std::endl;
    }
  }
};

} // namespace facebook::fboss
