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


namespace facebook { namespace fboss {

Wedge40Platform::InitPortMap Wedge40Platform::initPorts() {

  InitPortMap ports;

  auto add_port = [&](int num) {
      PortID portID(num);
      opennsl_port_t bcmPortNum = num;

      auto port = folly::make_unique<Wedge40Port>(portID);;
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
    add_ports(16 * 4);
    if (mode == WedgePlatformMode::LC) {
      // On LC, another 16 ports for back plane ports
      add_ports(16);
    }
  } else {
    // On FC, 32 40G ports
    add_ports(32);
  }

  return ports;
}

}} // namespace facebook::fboss
