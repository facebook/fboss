// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/link_tests/LinkTest.h"

int main(int argc, char* argv[]) {
  std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt;
#if defined(TAJO_SDK_VERSION_1_62_0) || defined(TAJO_SDK_VERSION_1_65_0)
  streamType = facebook::fboss::cfg::StreamType::UNICAST;
#endif
  return facebook::fboss::linkTestMain(
      argc, argv, facebook::fboss::initSaiPlatform, streamType);
}
