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

namespace facebook { namespace fboss {

class BcmPort;

typedef boost::container::flat_set<cfg::PortSpeed> LaneSpeeds;

/*
 * Struct for transmitter Equalization control settings
 * Applies to all broadcom platform port
 */
class TxSettings {
 public:
  TxSettings(uint8_t _driveCurrent,
             uint8_t _preTap,
             uint8_t _mainTap,
             uint8_t _postTap)
    : driveCurrent(_driveCurrent),
      preTap(_preTap),
      mainTap(_mainTap),
      postTap(_postTap) {}

  uint8_t driveCurrent{0};
  uint8_t preTap{0};
  uint8_t mainTap{0};
  uint8_t postTap{0};
};

class BcmPlatformPort : public PlatformPort {
 public:
  using XPEs = std::vector<unsigned int>;
  using TxOverrides = boost::container::flat_map<
    std::pair<TransmitterTechnology, double>, TxSettings>;

  BcmPlatformPort() {}
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

  /*
   * What type of cable is plugged in to the transceiver.
   */
  virtual folly::Future<TransmitterTechnology> getTransmitterTech(
      folly::EventBase* evb = nullptr) const = 0;

  /*
   * getTxSettings() returns the correct transmitter's amplitude control
   * parameter and Equalization control information.
   */
  virtual folly::Future<folly::Optional<TxSettings>> getTxSettings(
      folly::EventBase* evb = nullptr) const = 0;

  virtual const XPEs getEgressXPEs() const = 0;

  virtual bool shouldUsePortResourceAPIs() const = 0;
  virtual bool shouldSetupPortGroup() const = 0;

 private:
  // Forbidden copy constructor and assignment operator
  BcmPlatformPort(BcmPlatformPort const &) = delete;
  BcmPlatformPort& operator=(BcmPlatformPort const &) = delete;

  virtual TxOverrides getTxOverrides() const = 0;
};

}} // facebook::fboss
