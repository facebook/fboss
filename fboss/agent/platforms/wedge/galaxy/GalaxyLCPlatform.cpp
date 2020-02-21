/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/galaxy/GalaxyLCPlatform.h"

#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/galaxy/GalaxyPort.h"

namespace facebook::fboss {

std::unique_ptr<WedgePortMapping> GalaxyLCPlatform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
      {PortID(84), TransceiverID(0)},   {PortID(88), TransceiverID(1)},
      {PortID(92), TransceiverID(2)},   {PortID(96), TransceiverID(3)},
      {PortID(102), TransceiverID(4)},  {PortID(106), TransceiverID(5)},
      {PortID(110), TransceiverID(6)},  {PortID(114), TransceiverID(7)},
      {PortID(118), TransceiverID(8)},  {PortID(122), TransceiverID(9)},
      {PortID(126), TransceiverID(10)}, {PortID(130), TransceiverID(11)},
      {PortID(1), TransceiverID(12)},   {PortID(5), TransceiverID(13)},
      {PortID(9), TransceiverID(14)},   {PortID(13), TransceiverID(15)},
      {PortID(68), std::nullopt},       {PortID(72), std::nullopt},
      {PortID(76), std::nullopt},       {PortID(80), std::nullopt},
      {PortID(50), std::nullopt},       {PortID(54), std::nullopt},
      {PortID(58), std::nullopt},       {PortID(62), std::nullopt},
      {PortID(34), std::nullopt},       {PortID(38), std::nullopt},
      {PortID(42), std::nullopt},       {PortID(46), std::nullopt},
      {PortID(17), std::nullopt},       {PortID(21), std::nullopt},
      {PortID(25), std::nullopt},       {PortID(29), std::nullopt}};
  return WedgePortMapping::create<
      WedgePortMappingT<GalaxyPlatform, GalaxyPort>>(this, ports);
}

} // namespace facebook::fboss
