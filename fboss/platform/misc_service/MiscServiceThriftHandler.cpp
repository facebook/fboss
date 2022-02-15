/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"
#include <folly/logging/xlog.h>
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss::platform::misc_service {

void MiscServiceThriftHandler::getFruid(
    MiscFruidReadResponse& response,
    bool uncached) {
  auto log = LOG_THRIFT_CALL(DBG1);
  response.fruidData_ref() = miscService_->getFruid(uncached);
}
} // namespace facebook::fboss::platform::misc_service
