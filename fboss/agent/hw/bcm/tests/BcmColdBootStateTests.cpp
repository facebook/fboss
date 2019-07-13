/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook {
namespace fboss {

TEST_F(BcmTest, allPortsDisabledOnColdBoot) {
  auto setup = [this] { return getProgrammedState(); };
  auto verify = [this] {
    for (auto portIdAndPort : *getHwSwitch()->getPortTable()) {
      EXPECT_FALSE(portIdAndPort.second->isEnabled());
    }
  };
  setup();
  verify();
}

} // namespace fboss
} // namespace facebook
