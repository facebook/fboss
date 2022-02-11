// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/Weutil.h"
#include <fboss/lib/platforms/PlatformProductInfo.h>
#include <folly/logging/xlog.h>
#include <memory>
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook::fboss::platform {

std::unique_ptr<WeutilInterface> get_plat_weutil(void) {
  facebook::fboss::PlatformProductInfo prodInfo{FLAGS_fruid_filepath};
  prodInfo.initialize();
  if (prodInfo.getMode() == PlatformMode::DARWIN) {
    return std::make_unique<WeutilDarwin>();
  }

  XLOG(INFO) << "The platform (" << toString(prodInfo.getMode())
             << ") is not supported" << std::endl;
  return nullptr;
}

} // namespace facebook::fboss::platform
