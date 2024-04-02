/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"

#include <folly/logging/xlog.h>

#include "fboss/lib/LogThriftCall.h"
#include "fboss/platform/weutil/Weutil.h"

namespace facebook::fboss::platform::data_corral_service {

void DataCorralServiceThriftHandler::getFruid(
    DataCorralFruidReadResponse& response,
    bool uncached) {
  auto log = LOG_THRIFT_CALL(DBG1);

  std::vector<FruIdData> vData;

  if (uncached || fruid_.empty()) {
    fruid_ = createWeUtilIntf(
                 "chassis" /*eepromName*/, "" /*eepromPath*/, 0 /*offset*/)
                 ->getContents();
  }

  for (const auto& it : fruid_) {
    FruIdData data;
    data.name() = it.first;
    data.value() = it.second;
    vData.emplace_back(data);
  }

  response.fruidData() = vData;
}
} // namespace facebook::fboss::platform::data_corral_service
