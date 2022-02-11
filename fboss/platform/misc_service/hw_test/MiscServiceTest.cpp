/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/misc_service/hw_test/MiscServiceTest.h"

namespace facebook::fboss::platform::misc_service {

void MiscServiceTest::SetUp() {
  miscServiceImpl_ = std::make_unique<MiscServiceImpl>();
}
} // namespace facebook::fboss::platform::misc_service
