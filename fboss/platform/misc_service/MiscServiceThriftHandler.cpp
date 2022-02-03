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
#include "common/time/Time.h"
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss::platform::misc_service {

folly::coro::Task<std::unique_ptr<MiscFruidReadResponse>>
MiscServiceThriftHandler::co_getFruid(bool force) {
  auto log = LOG_THRIFT_CALL(DBG1);
  auto response = std::make_unique<MiscFruidReadResponse>();

  if (force) {
    // ToDo force a read
  } else {
    // ToDo get data from cache
  }

  co_return response;
}
} // namespace facebook::fboss::platform::misc_service
