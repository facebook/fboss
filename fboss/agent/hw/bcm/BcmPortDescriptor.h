// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <opennsl/l3.h>
#include <opennsl/types.h>
}

#include "fboss/agent/state/PortDescriptorTemplate.h"
#include "fboss/agent/types.h"

FBOSS_STRONG_TYPE(opennsl_port_t, BcmPortId)
FBOSS_STRONG_TYPE(opennsl_trunk_t, BcmTrunkId)

namespace facebook {
namespace fboss {

class BcmPortDescriptor : public PortDescriptorTemplate<BcmPortId, BcmTrunkId> {
 public:
  using BaseT = PortDescriptorTemplate<BcmPortId, BcmTrunkId>;
  explicit BcmPortDescriptor(BcmPortId port) : BaseT(port) {}
  explicit BcmPortDescriptor(BcmTrunkId trunk) : BaseT(trunk) {}
  opennsl_gport_t asGPort() const;
};

inline std::ostream& operator<<(
    std::ostream& out,
    const BcmPortDescriptor& pd) {
  return out << pd.str();
}

} // namespace fboss
} // namespace facebook
