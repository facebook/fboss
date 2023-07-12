// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/Weutil.h"

#include <folly/logging/xlog.h>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/platform/weutil/WeutilImpl.h"

namespace facebook::fboss::platform {

std::unique_ptr<WeutilInterface> get_plat_weutil(std::string eeprom) {
  facebook::fboss::PlatformProductInfo prodInfo{FLAGS_fruid_filepath};

  prodInfo.initialize();

  if (prodInfo.getType() == PlatformType::PLATFORM_DARWIN) {
    std::unique_ptr<WeutilDarwin> pDarwinIntf;
    pDarwinIntf = std::make_unique<WeutilDarwin>(eeprom);
    if (pDarwinIntf->verifyOptions()) {
      return std::move(pDarwinIntf);
    } else {
      return nullptr;
    }
  } else {
    std::unique_ptr<WeutilImpl> pWeutilImpl =
        std::make_unique<WeutilImpl>(eeprom);
    if (pWeutilImpl->verifyOptions()) {
      return std::move(pWeutilImpl);
    } else {
      return nullptr;
    }
  }

  XLOG(INFO) << "The platform (" << toString(prodInfo.getType())
             << ") is not supported" << std::endl;
  return nullptr;
}

} // namespace facebook::fboss::platform
