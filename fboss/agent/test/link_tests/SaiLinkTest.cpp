// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/logging/Init.h>
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/link_tests/LinkTest.h"

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");
#endif

int main(int argc, char* argv[]) {
  std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt;
#if defined(TAJO_SDK_GTE_24_4_90)
  streamType = facebook::fboss::cfg::StreamType::UNICAST;
#endif
  return facebook::fboss::linkTestMain(
      argc, argv, facebook::fboss::initSaiPlatform, streamType);
}
