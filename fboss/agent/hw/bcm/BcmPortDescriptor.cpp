// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmPortDescriptor.h"

#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"

namespace facebook::fboss {

bcm_gport_t BcmPortDescriptor::asGPort() const {
  switch (type()) {
    case PortType::PHYSICAL:
      return BcmPort::asGPort(phyPortID());
    case PortType::AGGREGATE:
      return BcmTrunk::asGPort(aggPortID());
  }
  throw FbossError("Unknown type for BcmPortDescriptor");
}

} // namespace facebook::fboss
