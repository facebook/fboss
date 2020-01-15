// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include "fboss/agent/state/PortDescriptorTemplate.h"
#include "fboss/agent/types.h"

FBOSS_STRONG_TYPE(bcm_port_t, BcmPortId)
FBOSS_STRONG_TYPE(bcm_trunk_t, BcmTrunkId)

namespace facebook::fboss {

class BcmPortDescriptor : public PortDescriptorTemplate<BcmPortId, BcmTrunkId> {
 public:
  using BaseT = PortDescriptorTemplate<BcmPortId, BcmTrunkId>;
  explicit BcmPortDescriptor(BcmPortId port) : BaseT(port) {}
  explicit BcmPortDescriptor(BcmTrunkId trunk) : BaseT(trunk) {}
  bcm_gport_t asGPort() const;
};

} // namespace facebook::fboss
