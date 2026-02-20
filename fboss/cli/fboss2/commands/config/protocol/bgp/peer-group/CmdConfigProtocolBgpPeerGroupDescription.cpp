/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupDescription.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupDescriptionTraits::RetType
CmdConfigProtocolBgpPeerGroupDescription::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameMsg,
    const ObjectArgType& descriptionMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  if (descriptionMsg.data().empty()) {
    return "Error: description is required";
  }

  std::string groupName = groupNameMsg.data()[0];
  std::string description = descriptionMsg.data()[0];

  session.setPeerGroupDescription(groupName, description);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer-group {} description to: {}\nConfig saved to: {}",
      groupName,
      description,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupDescription::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
