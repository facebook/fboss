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

// Parses the two positional arguments of
//   config switch admin-distance <client-id> <distance>
// where <client-id> is a routing ClientID (e.g. 0=BGP, 786=OpenR)
// and <distance> is the administrative distance to associate with it.
// ClientIDs 1 (STATIC_ROUTE), 2 (INTERFACE_ROUTE), 3 (LINKLOCAL_ROUTE),
// and 4 (REMOTE_INTERFACE_ROUTE) are forbidden: their admin distances are
// hardcoded in the agent and cannot be changed via config.
class AdminDistanceArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ AdminDistanceArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  int32_t getClientId() const {
    return clientId_;
  }

  int32_t getDistance() const {
    return distance_;
  }

 private:
  int32_t clientId_{0};
  int32_t distance_{0};
};

struct CmdConfigAdminDistanceTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "client_distance",
        args,
        "<client-id> <distance> - admin distance for a routing client "
        "(client-id: 0=BGP, 786=OpenR; distance: 0-255). "
        "Client-ids 1/2/3/4 are hardcoded and cannot be changed.");
  }
  using ObjectArgType = AdminDistanceArg;
  using RetType = std::string;
};

class CmdConfigAdminDistance
    : public CmdHandler<CmdConfigAdminDistance, CmdConfigAdminDistanceTraits> {
 public:
  using ObjectArgType = CmdConfigAdminDistanceTraits::ObjectArgType;
  using RetType = CmdConfigAdminDistanceTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& arg);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
