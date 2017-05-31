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
  // FC == Falcon Core
  // GalaxyFC is based on Tomahawk, which divides MMU buffer into 4
  // sub buffers called XPEs. FC are then mapped, to each XPE like so
  // Traffic egressing on FC0-15 use XPE 0, 2
  // Traffic egressing on FC16-13 use XPE 1, 3
  // Port to FC mappings are fixed for TH and are used as input to
  // creating bcm.conf files.
  WedgePortMapping::Port2TransceiverAndXPEs ports = {
    {PortID(72), TransceiverAndXPEs(none, {1, 3})}, // FC17->LC101
    {PortID(76), TransceiverAndXPEs(none, {1, 3})}, // FC18->LC101
    {PortID(68), TransceiverAndXPEs(none, {1, 3})}, // FC16->LC101
    {PortID(80), TransceiverAndXPEs(none, {1, 3})}, // FC19->LC101
    {PortID(62), TransceiverAndXPEs(none, {0, 2})}, // FC15->LC102
    {PortID(58), TransceiverAndXPEs(none, {0, 2})}, // FC14->LC102
    {PortID(54), TransceiverAndXPEs(none, {0, 2})}, // FC13->LC102
    {PortID(50), TransceiverAndXPEs(none, {0, 2})}, // FC12->LC102
    {PortID(110), TransceiverAndXPEs(none, {1, 3})}, // FC26->LC201
    {PortID(106), TransceiverAndXPEs(none, {1, 3})}, // FC25->LC201
    {PortID(102), TransceiverAndXPEs(none, {1, 3})}, // FC24->LC201
    {PortID(114), TransceiverAndXPEs(none, {1, 3})}, // FC27->LC201
    {PortID(96), TransceiverAndXPEs(none, {1, 3})}, // FC23->LC202
    {PortID(92), TransceiverAndXPEs(none, {1, 3})}, // FC22->LC202
    {PortID(84), TransceiverAndXPEs(none, {1, 3})}, // FC20->LC202
    {PortID(88), TransceiverAndXPEs(none, {1, 3})}, // FC21->LC202
    {PortID(5), TransceiverAndXPEs(none, {0, 2})}, // FC1->LC301
    {PortID(9), TransceiverAndXPEs(none, {0, 2})}, // FC2->LC301
    {PortID(1), TransceiverAndXPEs(none, {0, 2})}, // FC0->LC301
    {PortID(13), TransceiverAndXPEs(none, {0, 2})}, // FC3->LC301
    {PortID(130), TransceiverAndXPEs(none, {1, 3})}, // FC31->LC302
    {PortID(118), TransceiverAndXPEs(none, {1, 3})}, // FC28->LC302
    {PortID(122), TransceiverAndXPEs(none, {1, 3})}, // FC29->LC302
    {PortID(126), TransceiverAndXPEs(none, {1, 3})}, // FC30->LC302
    {PortID(42), TransceiverAndXPEs(none, {0, 2})}, // FC10->LC401
    {PortID(38), TransceiverAndXPEs(none, {0, 2})}, // FC9->LC401
    {PortID(29), TransceiverAndXPEs(none, {0, 2})}, // FC7->LC401
    {PortID(46), TransceiverAndXPEs(none, {0, 2})}, // FC11->LC401
    {PortID(34), TransceiverAndXPEs(none, {0, 2})}, // FC8->LC402
    {PortID(21), TransceiverAndXPEs(none, {0, 2})}, // FC5->LC402
    {PortID(25), TransceiverAndXPEs(none, {0, 2})}, // FC6->LC402
    {PortID(17), TransceiverAndXPEs(none, {0, 2})}, // FC4->LC402
  };
  return WedgePortMapping::create<WedgePortMappingT<GalaxyPort>>(this, ports);
}

}} // namespace facebook::fboss
