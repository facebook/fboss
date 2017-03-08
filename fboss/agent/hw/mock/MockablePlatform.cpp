/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockablePlatform.h"
#include "fboss/agent/hw/mock/MockableHwSwitch.h"
#include "fboss/agent/Platform.h"

#include <gmock/gmock.h>

using ::testing::Invoke;
using ::testing::_;

namespace facebook { namespace fboss {

MockablePlatform::MockablePlatform(std::shared_ptr<Platform> realPlatform)
    : MockPlatform(std::make_unique<::testing::NiceMock<MockableHwSwitch>>(
                     this, realPlatform->getHwSwitch())),
      realPlatform_(realPlatform) {
  // new functions added to Platform should invoke the corresponding function on
  // the real implementation here.
  ON_CALL(*this, createHandler(_))
    .WillByDefault(Invoke(realPlatform_.get(), &Platform::createHandler));
  ON_CALL(*this, getProductInfo(_))
    .WillByDefault(Invoke(realPlatform_.get(), &Platform::getProductInfo));
  ON_CALL(*this, getPortMapping(_))
    .WillByDefault(Invoke(realPlatform_.get(), &Platform::getPortMapping));
  ON_CALL(*this, getLocalMac())
    .WillByDefault(Invoke(realPlatform_.get(), &Platform::getLocalMac));
  ON_CALL(*this, onHwInitialized(_))
    .WillByDefault(Invoke(realPlatform_.get(), &Platform::onHwInitialized));
}

MockablePlatform::~MockablePlatform() {
}

}} // facebook::fboss
