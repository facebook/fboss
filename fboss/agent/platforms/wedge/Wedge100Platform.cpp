/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/Wedge100Platform.h"

#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/platforms/wedge/Wedge100Port.h"

#include <folly/Memory.h>

namespace facebook { namespace fboss {

Wedge100Platform::Wedge100Platform(
  std::unique_ptr<WedgeProductInfo> productInfo
) : WedgePlatform(std::move(productInfo)) {}

Wedge100Platform::~Wedge100Platform() {}

Wedge100Platform::InitPortMap Wedge100Platform::initPorts() {
  InitPortMap ports;

  auto add_port = [&](int num) {
      PortID portID(num);
      opennsl_port_t bcmPortNum = num;

      auto port = folly::make_unique<Wedge100Port>(portID);
      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
  };

  auto add_ports_stride = [&](int n_ports, int start, int stride) {
    int curr = start;
    while (n_ports--) {
      add_port(curr);
      curr += stride;
    }
  };

  add_ports_stride(8 * 4, 1, 1);
  add_ports_stride(8 * 4, 34, 1);
  add_ports_stride(8 * 4, 68, 1);
  add_ports_stride(8 * 4, 102, 1);

  return ports;
}

}} // facebook::fboss
