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

struct CmdShowTransceiverTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::map<int, facebook::fboss::TransceiverInfo>;
};

class CmdShowTransceiver
    : public CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits> {
 public:
  using ObjectArgType = CmdShowTransceiver::ObjectArgType;
  using RetType = CmdShowTransceiver::RetType;

  RetType queryClient(
      const folly::IPAddress& hostIp,
      const ObjectArgType& /* queriedPorts */) {
    auto client = utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(
        hostIp.str());

    RetType transceiverEntries;
    client->sync_getTransceiverInfo(transceiverEntries, {});

    return transceiverEntries;
  }

  void printOutput(const RetType& /* portId2TransceiverInfo */) {
    // TODO:
  }
};

} // namespace facebook::fboss
