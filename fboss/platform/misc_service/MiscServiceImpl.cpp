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

namespace {
// ToDo

} // namespace

namespace facebook::fboss::platform::misc_service {

void MiscServiceImpl::init() {
  // ToDo
  XLOG(INFO) << "Init MiscServiceImpl";
}

} // namespace facebook::fboss::platform::misc_service
