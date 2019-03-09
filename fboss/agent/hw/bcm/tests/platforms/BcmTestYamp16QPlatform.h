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

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestWedgeTomahawk3Platform.h"

namespace facebook { namespace fboss {
class BcmTestYamp16QPlatform : public BcmTestWedgeTomahawk3Platform {
public:
  BcmTestYamp16QPlatform();
  ~BcmTestYamp16QPlatform() override {}
  std::unique_ptr<BcmTestPort> getPlatformPort(PortID id) override;

private:
  // Forbidden copy constructor and assignment operator
  BcmTestYamp16QPlatform(BcmTestYamp16QPlatform const&) = delete;
  BcmTestYamp16QPlatform& operator=(BcmTestYamp16QPlatform const&) = delete;
};
}} // facebook::fboss
