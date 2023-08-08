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

#include <boost/container/flat_set.hpp>

#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <vector>

namespace facebook::fboss {

class BcmPort;
class BcmPlatform;

typedef boost::container::flat_set<cfg::PortSpeed> LaneSpeeds;

class BcmPlatformPort : public PlatformPort {
 public:
  using XPEs = std::vector<unsigned int>;
  using TxOverrides = boost::container::
      flat_map<std::pair<TransmitterTechnology, double>, phy::TxSettings>;

  BcmPlatformPort(PortID id, BcmPlatform* platform);
  BcmPlatformPort(BcmPlatformPort&&) = default;
  BcmPlatformPort& operator=(BcmPlatformPort&&) = default;

  /*
   * setBcmPort() will be called exactly once by the BCM code, during port
   * initialization.
   */
  virtual void setBcmPort(BcmPort* port) = 0;
  virtual BcmPort* getBcmPort() const = 0;

  /*
   * supportedLaneSpeeds() returns the all lane speeds supported on
   * this platform.
   */
  virtual LaneSpeeds supportedLaneSpeeds() const = 0;

  virtual const XPEs getEgressXPEs() const = 0;

  virtual bool shouldUsePortResourceAPIs() const = 0;

  virtual void updateStats() {}

 private:
  // Forbidden copy constructor and assignment operator
  BcmPlatformPort(BcmPlatformPort const&) = delete;
  BcmPlatformPort& operator=(BcmPlatformPort const&) = delete;
};

} // namespace facebook::fboss
