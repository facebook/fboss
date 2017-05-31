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
  // FC == Falcon Core
  // GalaxyLC is based on Tomahawk, which divides MMU buffer into 4
  // sub buffers called XPEs. FC are then mapped, to each XPE like so
  // Traffic egressing on FC0-15 use XPE 0, 2
  // Traffic egressing on FC16-13 use XPE 1, 3
  // Port to FC mappings are fixed for TH and are used as input to
  // creating bcm.conf files.
  WedgePortMapping::Port2TransceiverAndXPEs ports = {
    {PortID(84), TransceiverAndXPEs(TransceiverID(0), {1, 3})}, // FC20
    {PortID(88), TransceiverAndXPEs(TransceiverID(1), {1, 3})}, // FC21
    {PortID(92), TransceiverAndXPEs(TransceiverID(2), {1, 3})}, // FC22
    {PortID(96), TransceiverAndXPEs(TransceiverID(3), {1, 3})}, // FC23
    {PortID(102), TransceiverAndXPEs(TransceiverID(4), {1, 3})}, // FC24
    {PortID(106), TransceiverAndXPEs(TransceiverID(5), {1, 3})}, // FC25
    {PortID(110), TransceiverAndXPEs(TransceiverID(6), {1, 3})}, // FC26
    {PortID(114), TransceiverAndXPEs(TransceiverID(7), {1, 3})}, // FC27
    {PortID(118), TransceiverAndXPEs(TransceiverID(8), {1, 3})}, // FC28
    {PortID(122), TransceiverAndXPEs(TransceiverID(9), {1, 3})}, // FC29
    {PortID(126), TransceiverAndXPEs(TransceiverID(10), {1, 3})},// FC30
    {PortID(130), TransceiverAndXPEs(TransceiverID(11), {1, 3})}, // FC31
    {PortID(1), TransceiverAndXPEs(TransceiverID(12), {0, 2})}, // FC0
    {PortID(5), TransceiverAndXPEs(TransceiverID(13), {0, 2})}, // FC1
    {PortID(9), TransceiverAndXPEs(TransceiverID(14), {0, 2})}, // FC2
    {PortID(13), TransceiverAndXPEs(TransceiverID(15), {0, 2})},// FC3
    {PortID(68), TransceiverAndXPEs(none, {1, 3})}, // FC16
    {PortID(72), TransceiverAndXPEs(none, {1, 3})}, // FC17
    {PortID(76), TransceiverAndXPEs(none, {1, 3})}, // FC18
    {PortID(80), TransceiverAndXPEs(none, {1, 3})}, // FC19
    {PortID(50), TransceiverAndXPEs(none, {0, 2})}, // FC12
    {PortID(54), TransceiverAndXPEs(none, {0, 2})}, // FC13
    {PortID(58), TransceiverAndXPEs(none, {0, 2})}, // FC14
    {PortID(62), TransceiverAndXPEs(none, {0, 2})}, // FC15
    {PortID(34), TransceiverAndXPEs(none, {0, 2})}, // FC8
    {PortID(38), TransceiverAndXPEs(none, {0, 2})}, // FC9
    {PortID(42), TransceiverAndXPEs(none, {0, 2})}, // FC10
    {PortID(46), TransceiverAndXPEs(none, {0, 2})}, // FC11
    {PortID(17), TransceiverAndXPEs(none, {0, 2})}, // FC4
    {PortID(21), TransceiverAndXPEs(none, {0, 2})}, // FC5
    {PortID(25), TransceiverAndXPEs(none, {0, 2})}, // FC6
    {PortID(29), TransceiverAndXPEs(none, {0, 2})}  // FC7
  };
  return WedgePortMapping::create<WedgePortMappingT<GalaxyPort>>(this, ports);
}

}} // namespace facebook::fboss
