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
#include <utility>
#include <vector>

#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::utils {

// Custom type for PFC config key-value pairs
// Parses: <attr> <value> [<attr> <value> ...]
// where attr is one of: rx, tx, rx-duration, tx-duration,
// priority-group-policy, watchdog-detection-time, watchdog-recovery-action,
// watchdog-recovery-time
class PfcConfigAttrs : public BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ PfcConfigAttrs(std::vector<std::string> v);

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PFC_CONFIG_ATTRS;

 private:
  std::vector<std::pair<std::string, std::string>> attributes_;
};

} // namespace facebook::fboss::utils
