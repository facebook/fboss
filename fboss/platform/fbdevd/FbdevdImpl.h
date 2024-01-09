#pragma once

#include "fboss/platform/fbdevd/if/gen-cpp2/fbdevd_types.h"

namespace facebook::fboss::platform::fbdevd {

using namespace facebook::fboss::platform::fbdevd;

class FbdevdImpl {
 public:
  ~FbdevdImpl();
  explicit FbdevdImpl() {
    initPlatformConfig();
  }

 private:
  FbdevdConfig fbdevdConfig_;

  void initPlatformConfig();
};

} // namespace facebook::fboss::platform::fbdevd
