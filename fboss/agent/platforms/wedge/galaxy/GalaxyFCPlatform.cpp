/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/galaxy/GalaxyFCPlatform.h"

#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/galaxy/GalaxyPort.h"

namespace facebook::fboss {

std::unique_ptr<WedgePortMapping> GalaxyFCPlatform::createPortMapping() {
  using std::nullopt;

  WedgePortMapping::PortTransceiverMap ports = {
      {PortID(72), nullopt},  {PortID(76), nullopt},  {PortID(68), nullopt},
      {PortID(80), nullopt},  {PortID(62), nullopt},  {PortID(58), nullopt},
      {PortID(54), nullopt},  {PortID(50), nullopt},  {PortID(110), nullopt},
      {PortID(106), nullopt}, {PortID(102), nullopt}, {PortID(114), nullopt},
      {PortID(96), nullopt},  {PortID(92), nullopt},  {PortID(84), nullopt},
      {PortID(88), nullopt},  {PortID(5), nullopt},   {PortID(9), nullopt},
      {PortID(1), nullopt},   {PortID(13), nullopt},  {PortID(130), nullopt},
      {PortID(118), nullopt}, {PortID(122), nullopt}, {PortID(126), nullopt},
      {PortID(42), nullopt},  {PortID(38), nullopt},  {PortID(29), nullopt},
      {PortID(46), nullopt},  {PortID(34), nullopt},  {PortID(21), nullopt},
      {PortID(25), nullopt},  {PortID(17), nullopt},
  };
  return WedgePortMapping::create<
      WedgePortMappingT<GalaxyPlatform, GalaxyPort>>(this, ports);
}

} // namespace facebook::fboss
