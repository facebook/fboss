// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmNextHop.h"

#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook {
namespace fboss {

opennsl_if_t BcmL3NextHop::getEgressId() const {
  return hostReference_->getEgressId();
}

void BcmL3NextHop::programToCPU(opennsl_if_t intf) {
  auto* host = hostReference_->getBcmHost();
  host->programToCPU(intf);
}

bool BcmL3NextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

opennsl_if_t BcmMplsNextHop::getEgressId() const {
  return hostReference_->getEgressId();
}

void BcmMplsNextHop::programToCPU(opennsl_if_t intf) {
  auto* host = hostReference_->getBcmHost();
  host->programToCPU(intf);
}

bool BcmMplsNextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

BcmL3NextHop::BcmL3NextHop(BcmSwitch* hw, BcmHostKey key)
    : hw_(hw), key_(std::move(key)) {
  hostReference_ = BcmHostReference::get(hw_, key_);
  hostReference_->getEgressId(); // load reference
}

BcmMplsNextHop::BcmMplsNextHop(BcmSwitch* hw, BcmLabeledHostKey key)
    : hw_(hw), key_(std::move(key)) {
  hostReference_ = BcmHostReference::get(hw_, key_);
  hostReference_->getEgressId(); // load reference
}

} // namespace fboss
} // namespace facebook
