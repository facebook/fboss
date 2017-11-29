/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/GalaxyLCPlatform.h"
#include "fboss/agent/platforms/wedge/GalaxyPort.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"

using folly::none;

namespace facebook { namespace fboss {

std::unique_ptr<WedgePortMapping> GalaxyLCPlatform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
    {PortID(84), TransceiverID(0)},
    {PortID(88), TransceiverID(1)},
    {PortID(92), TransceiverID(2)},
    {PortID(96), TransceiverID(3)},
    {PortID(102), TransceiverID(4)},
    {PortID(106), TransceiverID(5)},
    {PortID(110), TransceiverID(6)},
    {PortID(114), TransceiverID(7)},
    {PortID(118), TransceiverID(8)},
    {PortID(122), TransceiverID(9)},
    {PortID(126), TransceiverID(10)},
    {PortID(130), TransceiverID(11)},
    {PortID(1), TransceiverID(12)},
    {PortID(5), TransceiverID(13)},
    {PortID(9), TransceiverID(14)},
    {PortID(13), TransceiverID(15)},
    {PortID(68), none},
    {PortID(72), none},
    {PortID(76), none},
    {PortID(80), none},
    {PortID(50), none},
    {PortID(54), none},
    {PortID(58), none},
    {PortID(62), none},
    {PortID(34), none},
    {PortID(38), none},
    {PortID(42), none},
    {PortID(46), none},
    {PortID(17), none},
    {PortID(21), none},
    {PortID(25), none},
    {PortID(29), none}
  };
  return WedgePortMapping::create<WedgePortMappingT<GalaxyPort>>(this, ports);
}

}} // namespace facebook::fboss
