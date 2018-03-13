/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAclRange.h"

#include <folly/Conv.h>

namespace facebook {
namespace fboss {

std::string AclRange::str() const {
  std::string ret;
  ret = "flags=";
  switch (flags_) {
    case SRC_L4_PORT:
      ret.append("SRC_L4_PORT");
      break;
    case DST_L4_PORT:
      ret.append("DST_L4_PORT");
      break;
    case PKT_LEN:
      ret.append("PKT_LEN");
      break;
  }
  ret.append(folly::to<std::string>(", min=", min_, ", max=", max_));
  return ret;
}

} // namespace fboss
} // namespace facebook
