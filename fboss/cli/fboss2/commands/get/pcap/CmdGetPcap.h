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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdGetPcapTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
  std::vector<utils::LocalOption> LocalOptions = {
      {"--name", "Name to identify packet capture"}};
};

class CmdGetPcap : public CmdHandler<CmdGetPcap, CmdGetPcapTraits> {
 public:
  using ObjectArgType = CmdGetPcapTraits::ObjectArgType;
  using RetType = CmdGetPcapTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::string nameStr;
    auto name =
        CmdLocalOptions::getInstance()->getLocalOption("get_pcap", "--name");
    nameStr =
        name.empty() ? "packet_capture.pcap" : fmt::format("{}.pcap", name);
    std::string remoteStr = fmt::format(
        "netops@{}:/var/facebook/fboss/captures/{}",
        hostInfo.getName(),
        nameStr);
    std::string cmdStr = fmt::format("scp {} .", remoteStr);
    utils::runCmd(cmdStr);
    return fmt::format(
        "Getting a copy of the packet capture \"{}\" from {}...",
        nameStr,
        hostInfo.getName());
  }

  void printOutput(
      const RetType& captureOutput,
      std::ostream& out = std::cout) {
    out << captureOutput << std::endl;
  }
};

} // namespace facebook::fboss
