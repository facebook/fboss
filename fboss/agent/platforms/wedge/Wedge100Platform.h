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

#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <boost/container/flat_map.hpp>
#include <memory>
#include <unordered_map>

namespace facebook { namespace fboss {

class BcmSwitch;
class WedgePort;

class Wedge100Platform : public WedgePlatform {
 public:
  explicit Wedge100Platform(std::unique_ptr<WedgeProductInfo> productInfo);
  ~Wedge100Platform() override;

  InitPortMap initPorts() override;

 private:
  Wedge100Platform(Wedge100Platform const &) = delete;
  Wedge100Platform& operator=(Wedge100Platform const &) = delete;

  std::map<std::string, std::string> loadConfig() override;
  std::unique_ptr<BaseWedgeI2CBus> getI2CBus() override;
  PortID fbossPortForQsfpChannel(int transceiver, int channel) override;
};

}} // namespace facebook::fboss
