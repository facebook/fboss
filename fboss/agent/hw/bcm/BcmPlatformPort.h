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

#include <vector>

namespace facebook { namespace fboss {

class BcmPort;

typedef boost::container::flat_set<cfg::PortSpeed> LaneSpeeds;

class BcmPlatformPort : public PlatformPort {
 public:
  using XPEs = std::vector<unsigned int>;

  explicit BcmPlatformPort(const XPEs& egressXPEs)
      : egressXPEs_(egressXPEs) {}
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

  const XPEs&  getEgressXPEs() const { return egressXPEs_; }

 private:
  // Forbidden copy constructor and assignment operator
  BcmPlatformPort(BcmPlatformPort const &) = delete;
  BcmPlatformPort& operator=(BcmPlatformPort const &) = delete;
  /*
   * Tomahawk onwards BRCM started dividing ASIC MMU into
   * multiple blocks called XPEs. A subset of ports then
   * get mapped to each XPE. For earlier ASICs (TD2),
   * where the MMU is not subdivided, we consider entire
   * MMU to be a single XPE
  */
  XPEs egressXPEs_;
};

}} // facebook::fboss
