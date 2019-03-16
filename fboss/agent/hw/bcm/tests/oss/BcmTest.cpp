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

#include "fboss/agent/FbossError.h"
/*
 * Linking with BCM libs requires us to define these symbols in our
 * application. For actual wedge_agent code we define these in
 * BcmFacebookAPI.cpp. For test functions, rather than pulling in
 * all BCM  api code just define these symbols here and have tests link
 */

extern "C" {
struct ibde_t;
ibde_t* bde;

int bde_create() {
  return 0;
}
void sal_config_init_defaults() {
}
}
namespace facebook {
namespace fboss {


void BcmTest::recreateHwSwitchFromWBState() {
  // noop
}

std::unique_ptr<std::thread> BcmTest::createThriftThread() const {
  throw FbossError(
      "Starting thrift server not supported");
}

} // namespace fboss
} // namespace facebook
