/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/set/fanhold/CmdSetFanHold.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdSetFanHold::RetType CmdSetFanHold::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& arg) {
  try {
    auto fan_service = utils::createClient<
        apache::thrift::Client<platform::fan_service::FanService>>(hostInfo);

    platform::fan_service::PwmHoldRequest holdReq;
    holdReq.pwm().from_optional(arg.pwm);
    fan_service->sync_setPwmHold(holdReq);

    return createModel(arg.pwm);
  } catch (const std::exception& e) {
    std::cerr << "Error fetching fan_service data: " << e.what() << std::endl;
    return {};
  }
}

CmdSetFanHold::RetType CmdSetFanHold::createModel(
    const std::optional<int>& pwm) {
  if (pwm.has_value()) {
    return folly::to<std::string>("Fan hold PWM was set to ", pwm.value());
  } else {
    return "Fan hold disabled";
  }
}

void CmdSetFanHold::printOutput(const RetType& model, std::ostream& out) {
  out << model << std::endl;
}

template void CmdHandler<CmdSetFanHold, CmdSetFanHoldTraits>::run();
} // namespace facebook::fboss
