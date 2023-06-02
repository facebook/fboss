/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/CommonPortUtils.h"
#include <folly/Format.h>
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace facebook::fboss {

TEST(CommonPortUtils, GetPimIds) {
  // Check Minipack2 case
  for (int pimId = 2; pimId <= 9; pimId++) {
    for (int pimTcvr = 1; pimTcvr <= 16; pimTcvr++) {
      auto portName = folly::sformat("eth{:d}/{:d}/1", pimId, pimTcvr);
      auto returnedPimId = getPimID(portName);
      CHECK_EQ(returnedPimId, pimId);
    }
  }
  // Check fab case
  for (int pimTcvr = 1; pimTcvr <= 48; pimTcvr++) {
    auto portName = folly::sformat("fab1/{:d}/1", pimTcvr);
    auto returnedPimId = getPimID(portName);
    CHECK_EQ(returnedPimId, 1);
  }
}

TEST(CommonPortUtils, GetTcvrIds) {
  // Check Minipack2 case
  for (int pimId = 2; pimId <= 9; pimId++) {
    for (int pimTcvr = 1; pimTcvr <= 16; pimTcvr++) {
      auto portName = folly::sformat("eth{:d}/{:d}/1", pimId, pimTcvr);
      auto returnedTcvrInPim = getTransceiverIndexInPim(portName);
      CHECK_EQ(returnedTcvrInPim, pimTcvr - 1);
    }
  }
  // Check FSW case
  for (int pimTcvr = 1; pimTcvr <= 48; pimTcvr++) {
    auto portName = folly::sformat("fab1/{:d}/1", pimTcvr);
    auto returnedTcvrInPim = getTransceiverIndexInPim(portName);
    CHECK_EQ(returnedTcvrInPim, pimTcvr - 1);
  }
}

} // namespace facebook::fboss
