/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockPlatform.h"

#include <folly/Memory.h>
#include "fboss/agent/SysError.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockTestHandle.h"

#include <gmock/gmock.h>

using std::make_unique;
using std::string;
using std::unique_ptr;

namespace facebook { namespace fboss {

MockPlatform::MockPlatform(std::unique_ptr<MockHwSwitch> hw) :
    tmpDir_("fboss_mock_state"),
    hw_(std::move(hw)) {
}

MockPlatform::MockPlatform() :
    MockPlatform(make_unique<::testing::NiceMock<MockHwSwitch>>(this)) {
}

MockPlatform::~MockPlatform() {
}

HwSwitch* MockPlatform::getHwSwitch() const {
  return hw_.get();
}

string MockPlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

string MockPlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

std::unique_ptr<HwTestHandle> MockPlatform::createTestHandle(
    std::unique_ptr<SwSwitch> sw) {
  return make_unique<MockTestHandle>(std::move(sw), this);
}


}} // facebook::fboss
