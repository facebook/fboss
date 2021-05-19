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

#include "fboss/lib/usb/WedgeI2CBus.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook {
namespace fboss {
class Wedge100Manager : public WedgeManager {
 public:
  Wedge100Manager();
  ~Wedge100Manager() override {}
  int getNumQsfpModules() override {
    return 32;
  }

 private:
  // Forbidden copy constructor and assignment operator
  Wedge100Manager(Wedge100Manager const&) = delete;
  Wedge100Manager& operator=(Wedge100Manager const&) = delete;

  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
};
} // namespace fboss
} // namespace facebook
