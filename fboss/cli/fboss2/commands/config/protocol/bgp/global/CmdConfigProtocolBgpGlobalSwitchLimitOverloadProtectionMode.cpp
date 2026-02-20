/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionModeTraits::RetType
CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& modeStr) {
  auto& session = BgpConfigSession::getInstance();

  if (modeStr.data().empty()) {
    return "Error: overload-protection-mode value is required";
  }

  int32_t mode = std::stoi(modeStr.data()[0]);

  session.setSwitchLimitOverloadProtectionMode(mode);
  session.saveConfig();

  return fmt::format(
      "Successfully set switch_limit_config overload_protection_mode to: {}\nConfig saved to: {}",
      mode,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
