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
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/agent/QsfpModule.h"

#include <folly/Memory.h>

namespace {
constexpr auto kNumWedge100QsfpModules = 32;
}

namespace facebook { namespace fboss {

Wedge100Platform::Wedge100Platform(
  std::unique_ptr<WedgeProductInfo> productInfo
) : WedgePlatform(std::move(productInfo), kNumWedge100QsfpModules) {}

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

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return folly::make_unique<Wedge100I2CBus>();
}

PortID Wedge100Platform::fbossPortForQsfpChannel(int transceiver, int channel) {
  // TODO(aeckert): remove this hack. Instead the qsfp channel for a
  // port should either be generated as part of the port map above, or
  // passed in through the config somehow. This is a temporary fix to get the
  // channel mapping correct for wedge100.
  //
  // This is needed due to Tomahawk port numbering limitations and the setup on
  // wedge100. There are two things we need to adjust for:
  //
  // 1. The first four qsfp transceivers are in fact the last 4
  //    logically on the tomahawk.
  // 2. Each pipe (32 physical ports) has 33 allowed logical numbers and
  //    a reserved loopback port, except the first pipe which skips 0 as
  //    an allowed port.
  //
  CHECK(transceiver >= 0 && transceiver < kNumWedge100QsfpModules);
  int th_quad = transceiver - 4;
  if (th_quad < 0) {
    // account for first 4 qsfps actually being connected to the last
    // ports on the tomahawk.
    th_quad += 32;
  }

  int pipe = th_quad / 8;
  int local = th_quad % 8;
  int port_num = pipe * 34 + local * QsfpModule::CHANNEL_COUNT + channel;
  if (pipe == 0) {
    // port 0 is reserved specially for the cpu port so increment one more.
    ++port_num;
  }

  return PortID(port_num);
}

}} // facebook::fboss
