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
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"

namespace facebook { namespace fboss {

std::unique_ptr<WedgePortMapping> Wedge40Platform::createPortMapping() {
  const WedgePortMapping::PortTransceiverMap ports = {
    {PortID(1), TransceiverID(0)},
    {PortID(5), TransceiverID(1)},
    {PortID(9), TransceiverID(2)},
    {PortID(13), TransceiverID(3)},
    {PortID(17), TransceiverID(4)},
    {PortID(21), TransceiverID(5)},
    {PortID(25), TransceiverID(6)},
    {PortID(29), TransceiverID(7)},
    {PortID(33), TransceiverID(8)},
    {PortID(37), TransceiverID(9)},
    {PortID(41), TransceiverID(10)},
    {PortID(45), TransceiverID(11)},
    {PortID(49), TransceiverID(12)},
    {PortID(53), TransceiverID(13)},
    {PortID(57), TransceiverID(14)},
    {PortID(61), TransceiverID(15)}
  };
  return WedgePortMapping::create<WedgePortMappingT<Wedge40Port>>(this, ports);
}

}} // namespace facebook::fboss
