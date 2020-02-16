/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge40/FakeWedge40Platform.h"

using std::make_unique;

namespace facebook::fboss {

std::string FakeWedge40Platform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

std::string FakeWedge40Platform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

} // namespace facebook::fboss
