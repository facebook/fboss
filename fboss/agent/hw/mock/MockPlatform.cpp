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
#include "fboss/agent/hw/mock/MockHwSwitch.h"

using folly::make_unique;
using std::string;
using std::unique_ptr;

namespace facebook { namespace fboss {

MockPlatform::MockPlatform()
  : tmpDir_("fboss_mock_state"),
    hw_(new MockHwSwitch(this)) {
}

MockPlatform::~MockPlatform() {
}

HwSwitch* MockPlatform::getHwSwitch() const {
  return hw_.get();
}

unique_ptr<ThriftHandler> MockPlatform::createHandler(SwSwitch* sw) {
  // We may need to implement this eventually.
  // For now none of our tests call createHandler()
  throw std::runtime_error("MockPlatform::createHandler() "
                           "not implemented yet");
}

string MockPlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

string MockPlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

}} // facebook::fboss
