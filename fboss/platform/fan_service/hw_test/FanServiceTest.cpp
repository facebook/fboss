/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fan_service/hw_test/FanServiceTest.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/platform/helpers/Init.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::fboss::platform {

void FanServiceTest::SetUp() {
  // Define service and handler for testing.
  std::tie(thriftServer_, thriftHandler_) = setupThrift();
  thriftHandler_->getFanService()->kickstart();
  // Finally, if this is for Meta, start service
  fsTestSetUp(thriftServer_, thriftHandler_.get());
}
void FanServiceTest::TearDown() {
  fsTestTearDown(thriftServer_, thriftHandler_.get());
}
} // namespace facebook::fboss::platform
