// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include "fboss/platform/weutil/WeutilInterface.h"

namespace facebook::fboss::platform {
class WeutilDarwin : public WeutilInterface {
 public:
  WeutilDarwin();
  void printInfo() override;
  void printInfoJson() override;
};

} // namespace facebook::fboss::platform
