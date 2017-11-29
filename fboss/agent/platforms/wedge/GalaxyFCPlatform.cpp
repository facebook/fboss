/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/GalaxyFCPlatform.h"
#include "fboss/agent/platforms/wedge/GalaxyPort.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"

using folly::none;

namespace facebook { namespace fboss {

std::unique_ptr<WedgePortMapping> GalaxyFCPlatform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
    {PortID(72), none},
    {PortID(76), none},
    {PortID(68), none},
    {PortID(80), none},
    {PortID(62), none},
    {PortID(58), none},
    {PortID(54), none},
    {PortID(50), none},
    {PortID(110), none},
    {PortID(106), none},
    {PortID(102), none},
    {PortID(114), none},
    {PortID(96), none},
    {PortID(92), none},
    {PortID(84), none},
    {PortID(88), none},
    {PortID(5), none},
    {PortID(9), none},
    {PortID(1), none},
    {PortID(13), none},
    {PortID(130), none},
    {PortID(118), none},
    {PortID(122), none},
    {PortID(126), none},
    {PortID(42), none},
    {PortID(38), none},
    {PortID(29), none},
    {PortID(46), none},
    {PortID(34), none},
    {PortID(21), none},
    {PortID(25), none},
    {PortID(17), none},
  };
  return WedgePortMapping::create<WedgePortMappingT<GalaxyPort>>(this, ports);
}

}} // namespace facebook::fboss
