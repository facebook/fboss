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
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

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

  auto add_quad = [&](int start, TransceiverID frontPanel) {
    for (int i = 0; i < 4; i++) {
      int num = start + i;
      PortID portID(num);
      opennsl_port_t bcmPortNum = num;

      auto port = std::make_unique<Wedge100Port>(portID, this, frontPanel,
                                                   ChannelID(i));
      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
    }
  };

  for (auto mapping : getFrontPanelMapping()) {
    add_quad(mapping.second, mapping.first);
  }

  return ports;
}

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return std::make_unique<Wedge100I2CBus>();
}

Wedge100Platform::FrontPanelMapping Wedge100Platform::getFrontPanelMapping() {
  // TODO(aeckert): move this mechanism in to WedgePlatform so that we can
  // initialize transceivers based on this for all wedge platforms.
  return {
    {TransceiverID(0), PortID(118)},
    {TransceiverID(1), PortID(122)},
    {TransceiverID(2), PortID(126)},
    {TransceiverID(3), PortID(130)},
    {TransceiverID(4), PortID(1)},
    {TransceiverID(5), PortID(5)},
    {TransceiverID(6), PortID(9)},
    {TransceiverID(7), PortID(13)},
    {TransceiverID(8), PortID(17)},
    {TransceiverID(9), PortID(21)},
    {TransceiverID(10), PortID(25)},
    {TransceiverID(11), PortID(29)},
    {TransceiverID(12), PortID(34)},
    {TransceiverID(13), PortID(38)},
    {TransceiverID(14), PortID(42)},
    {TransceiverID(15), PortID(46)},
    {TransceiverID(16), PortID(50)},
    {TransceiverID(17), PortID(54)},
    {TransceiverID(18), PortID(58)},
    {TransceiverID(19), PortID(62)},
    {TransceiverID(20), PortID(68)},
    {TransceiverID(21), PortID(72)},
    {TransceiverID(22), PortID(76)},
    {TransceiverID(23), PortID(80)},
    {TransceiverID(24), PortID(84)},
    {TransceiverID(25), PortID(88)},
    {TransceiverID(26), PortID(92)},
    {TransceiverID(27), PortID(96)},
    {TransceiverID(28), PortID(102)},
    {TransceiverID(29), PortID(106)},
    {TransceiverID(30), PortID(110)},
    {TransceiverID(31), PortID(114)}
  };
}

Wedge100Port* Wedge100Platform::getPortFromFrontPanelNum(TransceiverID fpPort) {
  auto iter = getFrontPanelMapping().find(fpPort);
  if (iter == getFrontPanelMapping().end()) {
    throw FbossError("Cannot find the port ID for front panel port ", fpPort);
  }
  // Could do a dynamic_cast, but we know the type is Wedge100Port*
  return static_cast<Wedge100Port*>(getPort(iter->second));
}

void Wedge100Platform::enableLedMode() {
  uint8_t mode = TWELVE_BIT_MODE;
  try {
    WedgeI2CBusLock(getI2CBus()).write(ADDR_SYSCPLD, LED_MODE_REG, 1, &mode);
  } catch (const std::exception& ex) {
    LOG(ERROR) << __func__ << ": failed to change LED mode: "
               << folly::exceptionStr(ex);
  }
}

void Wedge100Platform::onHwInitialized(SwSwitch* sw) {
  WedgePlatform::onHwInitialized(sw);
  enableLedMode();
}

void Wedge100Platform::onUnitAttach(int unit) {
  setPciPreemphasis(unit);
}

}} // facebook::fboss
