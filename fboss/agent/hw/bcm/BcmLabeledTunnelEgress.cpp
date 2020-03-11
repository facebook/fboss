// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"

#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Interface.h"

extern "C" {
#include <bcm/l3.h>
}

namespace {
facebook::fboss::LabelForwardingAction::LabelStack pushStack(
    const facebook::fboss::LabelForwardingAction::LabelStack& stack) {
  // skip the first label on stack, this is the bottom most label
  // this label is associated with labeled egress, and so the tunnel does not
  // have to include it in the label stack
  return facebook::fboss::LabelForwardingAction::LabelStack(
      stack.begin() + 1, stack.end());
}
} // namespace
namespace facebook::fboss {

BcmLabeledTunnelEgress::BcmLabeledTunnelEgress(
    const BcmSwitchIf* hw,
    bcm_mpls_label_t label,
    bcm_if_t interface,
    const LabelForwardingAction::LabelStack& labelStack)
    : BcmLabeledEgress(hw, label),
      tunnel_(hw->getIntfTable()->getBcmIntf(interface)->getBcmLabeledTunnel(
          pushStack(labelStack))) {
  CHECK(tunnel_);
}

BcmLabeledTunnelEgress::~BcmLabeledTunnelEgress() {}

BcmWarmBootCache::EgressId2EgressCitr BcmLabeledTunnelEgress::findEgress(
    bcm_vrf_t vrf,
    bcm_if_t intfId,
    const folly::IPAddress& ip) const {
  const auto& tunnelStack = tunnel_->getTunnelStack();
  LabelForwardingAction::LabelStack labels;
  labels.push_back(getLabel());
  std::copy(
      std::begin(tunnelStack),
      std::end(tunnelStack),
      std::back_inserter(labels));

  auto* bcmIntf = hw_->getIntfTable()->getBcmIntf(intfId);
  return hw_->getWarmBootCache()->findEgressFromLabeledHostKey(
      BcmLabeledHostKey(
          vrf, std::move(labels), ip, bcmIntf->getInterface()->getID()));
}

void BcmLabeledTunnelEgress::prepareEgressObject(
    bcm_if_t intfId,
    bcm_port_t port,
    const std::optional<folly::MacAddress>& mac,
    RouteForwardAction action,
    bcm_l3_egress_t* eObj) const {
  BcmLabeledEgress::prepareEgressObject(intfId, port, mac, action, eObj);
  eObj->intf = tunnel_->getTunnelInterface();
}

} // namespace facebook::fboss
