/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <memory>
#include <vector>

extern "C" {
#include <sai.h>
#include <saistatus.h>
#include <saiswitch.h>
}

namespace facebook::fboss {

class SaiHwPlatform : public SaiPlatform {
 public:
  using SaiPlatform::SaiPlatform;
  std::string getPersistentStateDir() const override;
  std::string getVolatileStateDir() const override;
};

} // namespace facebook::fboss
