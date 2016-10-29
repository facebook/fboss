/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/StateUtils.h"

#include <folly/Format.h>

namespace facebook {
namespace fboss {
namespace StateUtils {

std::string getCpp2EnumName(const std::string& enumValue) {
  auto pos = enumValue.find("::");
  if (pos == std::string::npos) {
    return enumValue;
  }

  return enumValue.substr(pos + 2);
}

} // namespace StateUtils
} // namespace fboss
} // namespace facebook
