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

struct CmdShowPortTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    RetType retVal;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getAllPortInfo(retVal);
    return retVal;
  }

  void printOutput(const RetType& portInfoThrift) {
    // TODO
  }
};

} // namespace facebook::fboss
