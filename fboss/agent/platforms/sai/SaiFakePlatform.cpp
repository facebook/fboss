/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiFakePlatform.h"

#include <cstdio>
#include <cstring>
namespace facebook {
namespace fboss {

std::string SaiFakePlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

std::string SaiFakePlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

std::string SaiFakePlatform::getHwConfig() {
  return {};
}

} // namespace fboss
} // namespace facebook
