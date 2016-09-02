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

#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <folly/Range.h>
#include <memory>
#include <map>
#include <vector>

namespace facebook { namespace fboss {

class WedgeProductInfo;
class Wedge100Port;

class GalaxyPlatform : public WedgePlatform {
 public:
  explicit GalaxyPlatform(std::unique_ptr<WedgeProductInfo> productInfo);

  InitPortMap initPorts() override;
 private:
  static constexpr auto kNumFrontPanelPorts = 16;

  GalaxyPlatform(GalaxyPlatform const &) = delete;
  GalaxyPlatform& operator=(GalaxyPlatform const &) = delete;

  using FrontPanelMapping = std::map<TransceiverID, PortID>;
  using BackplanePorts = std::vector<PortID>;

  bool isLC() const { return getMode() == WedgePlatformMode::LC; }
  bool isFC() const { return getMode() == WedgePlatformMode::FC; }
  FrontPanelMapping getFrontPanelMapping() const;
  FrontPanelMapping getLCFrontPanelMapping() const;
  FrontPanelMapping getFCFrontPanelMapping() const;
  BackplanePorts getBackplanePorts() const;
  BackplanePorts getLCBackplanePorts() const;
  BackplanePorts getFCBackplanePorts() const;

  std::map<std::string, std::string> loadConfig() override;

  PortID fbossPortForQsfpChannel(int transceiver, int channel) override;
  folly::ByteRange defaultLed0Code() override;
  folly::ByteRange defaultLed1Code() override;

  FrontPanelMapping frontPanelMapping_;

};

}}
