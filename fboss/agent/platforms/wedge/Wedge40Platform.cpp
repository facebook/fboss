/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/Wedge40Platform.h"
#include "fboss/agent/platforms/wedge/Wedge40Port.h"

namespace {

constexpr int kWedge40FrontPanelPorts = 16;

}

namespace facebook { namespace fboss {

Wedge40Platform::Wedge40Platform(
    std::unique_ptr<WedgeProductInfo> productInfo,
    WedgePlatformMode mode)
    : WedgePlatform(
          std::move(productInfo),
          mode == WedgePlatformMode::FC ? 0 : kWedge40FrontPanelPorts) {
  CHECK(
      mode == WedgePlatformMode::FC || mode == WedgePlatformMode::LC ||
      mode == WedgePlatformMode::WEDGE);
}

Wedge40Platform::InitPortMap Wedge40Platform::initPorts() {

  InitPortMap ports;
  auto add_port = [&](int num,
        folly::Optional<TransceiverID> frontPanel=nullptr,
        folly::Optional<ChannelID> channel=nullptr) {
      PortID portID(num);
      opennsl_port_t bcmPortNum = num;

      auto port = folly::make_unique<Wedge40Port>(portID, frontPanel, channel);

      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
  };

  // Wedge has 16 QSFPs, each mapping to 4 physical ports.
  int portNum = 0;

  auto add_ports = [&](int n_ports) {
    while (n_ports--) {
      ++portNum;
      add_port(portNum);
    }
  };

  auto mode = getMode();
  if (mode == WedgePlatformMode::WEDGE || mode == WedgePlatformMode::LC) {
    // Front panel are 16 4x10G ports
    for (const auto& mapping : getFrontPanelMapping()) {
      for (int i = 0; i < 4; i++) {
        add_port(mapping.second + i,
            folly::Optional<TransceiverID>(mapping.first),
            folly::Optional<ChannelID>(ChannelID(i)));
      }
    }
    if (mode == WedgePlatformMode::LC) {
      portNum = getFrontPanelMapping().size() * 4;
      // On LC, another 16 ports for back plane ports
      // No transceivers or channels
      add_ports(16);
    }
  } else {
    // On FC, 32 40G ports
    add_ports(32);
  }

  return ports;
}

}} // namespace facebook::fboss
