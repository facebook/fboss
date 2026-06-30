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

class NeedL2EntryForNeighborArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */
  NeedL2EntryForNeighborArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  bool isEnabled() const {
    return enabled_;
  }

 private:
  bool enabled_ = true;
};

struct CmdConfigNeedL2EntryForNeighborTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "state",
           args,
           "Whether to require an L2 entry before installing a neighbor entry (enabled|disabled)")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = NeedL2EntryForNeighborArg;
  using RetType = std::string;
};

class CmdConfigNeedL2EntryForNeighbor
    : public CmdHandler<
          CmdConfigNeedL2EntryForNeighbor,
          CmdConfigNeedL2EntryForNeighborTraits> {
 public:
  using ObjectArgType = CmdConfigNeedL2EntryForNeighborTraits::ObjectArgType;
  using RetType = CmdConfigNeedL2EntryForNeighborTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& stateArg);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
