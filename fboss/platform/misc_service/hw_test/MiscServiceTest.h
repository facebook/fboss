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

#include <gtest/gtest.h>

#include "fboss/platform/misc_service/MiscServiceImpl.h"

namespace facebook::fboss::platform::misc_service {

class MiscServiceTest : public ::testing::Test {
 public:
  void SetUp() override;

 protected:
  std::unique_ptr<MiscServiceImpl> miscServiceImpl_;
};
} // namespace facebook::fboss::platform::misc_service
