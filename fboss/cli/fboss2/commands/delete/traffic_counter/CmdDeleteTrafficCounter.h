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

#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

// Parses the single positional argument of
//   delete traffic-counter <name>
class TrafficCounterNameArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ TrafficCounterNameArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getName() const {
    return name_;
  }

 private:
  std::string name_;
};

struct CmdDeleteTrafficCounterTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // required() + expected(1) keeps CLI11 from reclassifying the counter
    // name as a subcommand when it happens to match one elsewhere in the tree.
    cmd.add_option(
           "counter_name",
           args,
           "<name> - name of the traffic counter to delete")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = TrafficCounterNameArg;
  using RetType = std::string;
};

/*
 * Deletes a named traffic counter from SwitchConfig.trafficCounters.
 * Refuses (FbossError) while any traffic-policy match action still
 * references the counter — the referencing match actions must be removed
 * first, otherwise the config would no longer apply.
 */
class CmdDeleteTrafficCounter : public CmdHandler<
                                    CmdDeleteTrafficCounter,
                                    CmdDeleteTrafficCounterTraits> {
 public:
  using ObjectArgType = CmdDeleteTrafficCounterTraits::ObjectArgType;
  using RetType = CmdDeleteTrafficCounterTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& arg);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
