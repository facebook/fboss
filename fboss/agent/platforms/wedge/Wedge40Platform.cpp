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
  // TD2 does not divide MMU buffer into subregions. So we
  // consider the entire MMU buffer a XPE0 and map all ports
  // to it.
  const WedgePortMapping::Port2TransceiverAndXPEs ports = {
    {PortID(1), TransceiverAndXPEs(TransceiverID(0), {0})},
    {PortID(5), TransceiverAndXPEs(TransceiverID(1), {0})},
    {PortID(9), TransceiverAndXPEs(TransceiverID(2), {0})},
    {PortID(13), TransceiverAndXPEs(TransceiverID(3), {0})},
    {PortID(17), TransceiverAndXPEs(TransceiverID(4), {0})},
    {PortID(21), TransceiverAndXPEs(TransceiverID(5), {0})},
    {PortID(25), TransceiverAndXPEs(TransceiverID(6), {0})},
    {PortID(29), TransceiverAndXPEs(TransceiverID(7), {0})},
    {PortID(33), TransceiverAndXPEs(TransceiverID(8), {0})},
    {PortID(37), TransceiverAndXPEs(TransceiverID(9), {0})},
    {PortID(41), TransceiverAndXPEs(TransceiverID(10), {0})},
    {PortID(45), TransceiverAndXPEs(TransceiverID(11), {0})},
    {PortID(49), TransceiverAndXPEs(TransceiverID(12), {0})},
    {PortID(53), TransceiverAndXPEs(TransceiverID(13), {0})},
    {PortID(57), TransceiverAndXPEs(TransceiverID(14), {0})},
    {PortID(61), TransceiverAndXPEs(TransceiverID(15), {0})}
  };
  return WedgePortMapping::create<WedgePortMappingT<Wedge40Port>>(this, ports);
}

}} // namespace facebook::fboss
