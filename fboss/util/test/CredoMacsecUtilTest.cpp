// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/test/CredoMacsecUtilTest.h"
#include <folly/FileUtil.h>
#include <folly/Singleton.h>
#include "common/init/Init.h"
#include "fboss/util/CredoMacsecUtil.h"

using namespace facebook::fboss;
using namespace facebook::fboss::mka;
using namespace apache::thrift;

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}

namespace facebook {
namespace fboss {

void CredoMacsecUtilTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
}

void CredoMacsecUtilTest::TearDown() {}

TEST_F(CredoMacsecUtilTest, getMacsecSaFromJsonTest) {
  const std::string jsonTestFile =
      tmpDir.path().string() + "/getMacsecSaFromJsonTest";
  int fd = folly::openNoInt(jsonTestFile.c_str(), O_CREAT | O_RDWR | O_TRUNC);
  ASSERT_NE(fd, -1);

  const std::string jsonContent = R"(
  {
    "sci": {
      "macAddress": "2c-54-91-88-c9-e3",
      "port": 1
    },
    "l2Port": "2",
    "assocNum": 4,
    "keyHex": "0xA",
    "keyIdHex": "0xB",
    "primary": true
  })";
  folly::writeNoInt(fd, jsonContent.c_str(), jsonContent.length());
  folly::closeNoInt(fd);

  MKASak sak;
  CredoMacsecUtil macsec;
  bool rc = macsec.getMacsecSaFromJson(jsonTestFile, sak);
  ASSERT_EQ(rc, true);
  ASSERT_EQ(sak.sci()->get_macAddress(), "2c-54-91-88-c9-e3");
  ASSERT_EQ(sak.sci()->get_port(), 1);
  ASSERT_EQ(sak.get_l2Port(), "2");
  ASSERT_EQ(sak.get_assocNum(), 4);
  ASSERT_EQ(sak.get_keyHex(), "0xA");
  ASSERT_EQ(sak.get_keyIdHex(), "0xB");
  ASSERT_EQ(sak.get_primary(), true);
}

TEST_F(CredoMacsecUtilTest, getMacsecScFromJsonTest) {
  const std::string jsonTestFile =
      tmpDir.path().string() + "/getMacsecScFromJsonTest";
  int fd = folly::openNoInt(jsonTestFile.c_str(), O_CREAT | O_RDWR | O_TRUNC);
  ASSERT_NE(fd, -1);

  const std::string jsonContent = R"(
  {
    "macAddress": "E3-4F-11-D3-9A-32",
    "port": 3
  })";
  folly::writeNoInt(fd, jsonContent.c_str(), jsonContent.length());
  folly::closeNoInt(fd);

  MKASci sci;
  CredoMacsecUtil macsec;
  bool rc = macsec.getMacsecScFromJson(jsonTestFile, sci);
  ASSERT_EQ(rc, true);
  ASSERT_EQ(sci.get_macAddress(), "E3-4F-11-D3-9A-32");
  ASSERT_EQ(sci.get_port(), 3);
}

} // namespace fboss
} // namespace facebook
