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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/sflow/CmdConfigInterfaceSflow.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Parses the single positional argument of
//   config interface <name> sflow sample-dest <cpu|mirror>
class SampleDestValue : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ SampleDestValue( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  cfg::SampleDestination getDestination() const {
    return dest_;
  }

  // The CLI token the destination was parsed from ("cpu" / "mirror").
  const std::string& getToken() const {
    return token_;
  }

 private:
  cfg::SampleDestination dest_{cfg::SampleDestination::CPU};
  std::string token_;
};

struct CmdConfigInterfaceSflowSampleDestTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterfaceSflow;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "sample_dest",
        args,
        "<cpu|mirror> - where sFlow samples are sent: processed on-box (cpu) "
        "or sent to the port's ingress mirror (mirror)");
  }
  using ObjectArgType = SampleDestValue;
  using RetType = std::string;
};

class CmdConfigInterfaceSflowSampleDest
    : public CmdHandler<
          CmdConfigInterfaceSflowSampleDest,
          CmdConfigInterfaceSflowSampleDestTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceSflowSampleDestTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceSflowSampleDestTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& sampleDest);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
