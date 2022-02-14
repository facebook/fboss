/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/FileUtil.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/weutil/Weutil.h"

namespace {
// ToDo

} // namespace

namespace facebook::fboss::platform::misc_service {

void MiscServiceImpl::init() {
  // ToDo
  XLOG(INFO) << "Init MiscServiceImpl";
}

std::vector<FruIdData> MiscServiceImpl::getFruid(bool uncached) {
  std::vector<FruIdData> vData;

  if (uncached || fruid_.empty()) {
    fruid_ = get_plat_weutil()->getInfo();
  }

  for (auto it : fruid_) {
    FruIdData data;
    data.name_ref() = it.first;
    data.value_ref() = it.second;
    vData.emplace_back(data);
  }

  return vData;
}
} // namespace facebook::fboss::platform::misc_service
