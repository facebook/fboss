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
namespace facebook {
namespace fboss {

std::unique_ptr<std::thread> BcmTest::createThriftThread() const {
  throw FbossError("Starting thrift server not supported");
}

} // namespace fboss
} // namespace facebook
