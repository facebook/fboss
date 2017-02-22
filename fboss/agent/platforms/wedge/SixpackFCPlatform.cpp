/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/SixpackFCPlatform.h"
#include "fboss/agent/platforms/wedge/Wedge40Port.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"

using folly::none;

namespace facebook { namespace fboss {

std::unique_ptr<WedgePortMapping> SixpackFCPlatform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
    {PortID(1), none},
    {PortID(5), none},
    {PortID(9), none},
    {PortID(13), none},
    {PortID(17), none},
    {PortID(21), none},
    {PortID(25), none},
    {PortID(29), none},
    {PortID(33), none},
    {PortID(37), none},
    {PortID(41), none},
    {PortID(45), none},
    {PortID(49), none},
    {PortID(53), none},
    {PortID(57), none},
    {PortID(61), none},
    {PortID(65), none},
    {PortID(69), none},
    {PortID(73), none},
    {PortID(77), none},
    {PortID(81), none},
    {PortID(85), none},
    {PortID(89), none},
    {PortID(93), none},
    {PortID(97), none},
    {PortID(101), none},
    {PortID(105), none},
    {PortID(109), none},
    {PortID(113), none},
    {PortID(117), none},
    {PortID(121), none},
    {PortID(125), none}
  };
  return WedgePortMapping::create<WedgePortMappingT<Wedge40Port>>(this, ports);
}

}} // namespace facebook::fboss
