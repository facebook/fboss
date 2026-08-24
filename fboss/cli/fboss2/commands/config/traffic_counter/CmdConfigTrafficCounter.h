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

#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

// Args: <name> <types>, where types is a comma separated list of
// PACKETS,BYTES
class TrafficCounterArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ TrafficCounterArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getName() const {
    return name_;
  }

  const std::vector<cfg::CounterType>& getTypes() const {
    return types_;
  }

 private:
  std::string name_;
  std::vector<cfg::CounterType> types_;
};

struct CmdConfigTrafficCounterTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "name_and_types",
           args,
           "Counter name followed by a comma separated list of counter types (PACKETS,BYTES)")
        ->expected(2);
  }
  using ObjectArgType = TrafficCounterArg;
  using RetType = std::string;
};

class CmdConfigTrafficCounter : public CmdHandler<
                                    CmdConfigTrafficCounter,
                                    CmdConfigTrafficCounterTraits> {
 public:
  using ObjectArgType = CmdConfigTrafficCounterTraits::ObjectArgType;
  using RetType = CmdConfigTrafficCounterTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& trafficCounter);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
