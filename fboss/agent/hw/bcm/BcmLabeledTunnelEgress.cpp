// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace {
facebook::fboss::LabelForwardingAction::LabelStack pushStack(
    const facebook::fboss::LabelForwardingAction::LabelStack& stack) {
  return facebook::fboss::LabelForwardingAction::LabelStack(
      stack.begin() + 1, stack.end());
}
} // namespace
namespace facebook {
namespace fboss {

BcmLabeledTunnelEgress::BcmLabeledTunnelEgress(
    const BcmSwitchIf* hw,
    opennsl_mpls_label_t label,
    opennsl_if_t interface,
    const LabelForwardingAction::LabelStack& labelStack)
    : BcmLabeledEgress(hw, label),
      tunnel_(hw->getIntfTable()->getBcmIntf(interface)->getBcmLabeledTunnel(
          pushStack(labelStack))) {
  CHECK(tunnel_);
}

BcmLabeledTunnelEgress::~BcmLabeledTunnelEgress() {}

BcmWarmBootCache::EgressId2EgressCitr BcmLabeledTunnelEgress::findEgress(
    opennsl_vrf_t /*vrf*/,
    opennsl_if_t /*intfId*/,
    const folly::IPAddress& /*ip*/) const {
  // TODO(pshaikh) : support warmboot for labeled tunnel egress
  return hw_->getWarmBootCache()->egressId2Egress_end();
}

} // namespace fboss
} // namespace facebook
