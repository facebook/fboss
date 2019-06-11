/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/QsfpServiceTest.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "common/init/Init.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

using namespace facebook::fboss;

DEFINE_int32(thrift_port, 5910, "Port for the thrift service");

DEFINE_int32(
    loop_interval,
    5,
    "Interval (in seconds) to run the main loop that determines "
    "if we need to change or fetch data for transceivers");

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}

namespace facebook {
namespace fboss {

void QsfpServiceTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();

  transceiverManager_ = createTransceiverManager();
}

void QsfpServiceTest::TearDown() {}

TEST_F(QsfpServiceTest, simpleTest) {
  std::vector<int32_t> data = {1, 3, 7};
  std::map<int32_t, TransceiverInfo> info;

  transceiverManager_->getTransceiversInfo(
      info, std::make_unique<std::vector<int32_t>>(data));
  ASSERT_EQ(3, info.size());
}

} // namespace fboss
} // namespace facebook
