/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <glog/logging.h>
#include <gtest/gtest.h>

extern "C" {
#include <bcm/port.h>
}

#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

extern "C" {
struct ibde_t;
ibde_t* bde;
}

namespace facebook::fboss {

TEST(GPort, NonoverlappingRange) {
  bcm_gport_t asPhysicalPort, asTrunkPort;
  for (int p = 0; p < (1 << 16); ++p) {
    BCM_GPORT_TRUNK_SET(asTrunkPort, static_cast<bcm_trunk_t>(p));
    BCM_GPORT_LOCAL_SET(asPhysicalPort, static_cast<bcm_port_t>(p));
    CHECK_NE(asPhysicalPort, asTrunkPort);
  }
}

} // namespace facebook::fboss
