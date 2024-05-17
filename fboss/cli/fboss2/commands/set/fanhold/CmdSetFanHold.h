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
#include "fboss/cli/fboss2/commands/show/facebook/environment/fan/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/platform/fan_service/if/gen-cpp2/FanService.h"
#include "folly/json/json.h"

#include <fmt/core.h>
#include <re2/re2.h>
#include <cstdint>

namespace facebook::fboss {

using utils::Table;

/*
 Define the traits of this command. This will include the inputs and output
 types
*/
struct CmdSetFanHoldTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_FAN_PWM;
  using ObjectArgType = utils::FanPwm;
  using RetType = std::string;
};

class CmdSetFanHold : public CmdHandler<CmdSetFanHold, CmdSetFanHoldTraits> {
 public:
  using ObjectArgType = CmdSetFanHoldTraits::ObjectArgType;
  using RetType = CmdSetFanHoldTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& arg) {
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

  RetType createModel(const std::optional<int>& pwm) {
    if (pwm.has_value()) {
      return folly::to<std::string>("Fan hold PWM was set to ", pwm.value());
    } else {
      return "Fan hold disabled";
    }
  }

  /*
    Output to human readable format
  */
  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model << std::endl;
  }
};
} // namespace facebook::fboss
