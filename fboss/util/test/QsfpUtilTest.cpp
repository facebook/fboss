// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/test/QsfpUtilTest.h"
#include "fboss/util/wedge_qsfp_util.h"
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include "common/init/Init.h"

using namespace facebook::fboss;

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}

namespace facebook {
namespace fboss {

void QsfpUtilTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
}

void QsfpUtilTest::TearDown() {}

TEST_F(QsfpUtilTest, VerifyCheckSum) {

  CdbCommandBlock cdb;
  std::vector<uint8_t> lplMem;

  uint8_t* pSum = (uint8_t*)&cdb + 5;

  // For command 0x0100 the 1's command block checksum will be 0xfe
  cdb.createCdbCmdGeneric(0x0100, lplMem);
  ASSERT_EQ(*pSum, 0xfe);

  // For command 0x0043 the 1's command block checksum will be 0xbc
  cdb.createCdbCmdGeneric(0x0043, lplMem);
  ASSERT_EQ(*pSum, 0xbc);

  // For command 0x0042 the 1's command block checksum will be 0xbd
  cdb.createCdbCmdGeneric(0x0042, lplMem);
  ASSERT_EQ(*pSum, 0xbd);

  // For command 0x0000 with LPL memory bytes 100, 200 the 1's command block
  // the checksum will be 0xd1
  lplMem.push_back(100);
  lplMem.push_back(200);
  cdb.createCdbCmdGeneric(0, lplMem);
  ASSERT_EQ(*pSum, 0xd1);
}

} // namespace fboss
} // namespace facebook
