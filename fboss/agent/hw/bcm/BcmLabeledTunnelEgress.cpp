// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace {
auto constexpr kTunnel = "tunnel";
} // namespace
namespace facebook {
namespace fboss {

BcmLabeledTunnelEgress::BcmLabeledTunnelEgress(
    const BcmSwitchIf* hw,
    opennsl_mpls_label_t label,
    opennsl_if_t interface,
    const LabelForwardingAction::LabelStack& labelStack)
    : BcmLabeledEgress(hw, label),
      tunnel_(hw->getIntfTable()->getBcmIntf(
          interface)->getBcmLabeledTunnel(labelStack)) {
  CHECK(tunnel_);
}

BcmLabeledTunnelEgress::~BcmLabeledTunnelEgress() {}

folly::dynamic BcmLabeledTunnelEgress::toFollyDynamic() const {
  auto egress = BcmLabeledEgress::toFollyDynamic();
  egress[kTunnel] = tunnel_->toFollyDynamic();
  return egress;
}

} // namespace fboss
} // namespace facebook
