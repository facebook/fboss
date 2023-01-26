#pragma once

#include "fboss/platform/fbdevd/if/gen-cpp2/fbdevd_types.h"

namespace facebook::fboss::platform::fbdevd {

using namespace facebook::fboss::platform::fbdevd;

class FbdevdImpl {
 public:
  FbdevdImpl() {
    initPlatformConfig();
  }
  ~FbdevdImpl();
  explicit FbdevdImpl(const std::string& confFileName)
      : confFileName_{confFileName} {
    initPlatformConfig();
  }

 private:
  // Fbdevd config file full path
  std::string confFileName_{};

  fbossPlatformDesc fbdevdConfig_;

  void initPlatformConfig();
};

} // namespace facebook::fboss::platform::fbdevd
