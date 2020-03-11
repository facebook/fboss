// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/logging/xlog.h>

extern "C" {
#include <bcm/mpls.h>
}

namespace {
using namespace facebook::fboss;
std::string strLabelStack(const LabelForwardingAction::LabelStack& stack) {
  std::string out = "{ ";
  for (const auto& label : stack) {
    out += folly::to<std::string>(label);
    out += " ";
  }
  out += "}";
  return out;
}

void setupLabelStack(
    bcm_mpls_egress_label_t* labels,
    const LabelForwardingAction::LabelStack& stack,
    std::optional<BcmQosPolicyHandle> qosMapId) {
  for (auto i = 0; i < stack.size(); i++) {
    bcm_mpls_egress_label_t_init(&labels[i]);
    labels[i].label = stack[i];
    // copy ttl from l3 packet after decrementing it by while encapsulating
    labels[i].flags =
        (BCM_MPLS_EGRESS_LABEL_TTL_COPY | BCM_MPLS_EGRESS_LABEL_TTL_DECREMENT);
    if (qosMapId) {
      labels[i].flags |= BCM_MPLS_EGRESS_LABEL_EXP_REMARK;
      labels[i].qos_map_id = qosMapId.value();
    }
  }
}

} // namespace
namespace facebook::fboss {

BcmLabeledTunnel::BcmLabeledTunnel(
    BcmSwitch* hw,
    bcm_if_t l3Intf,
    LabelForwardingAction::LabelStack stack)
    : hw_(hw), stack_(std::move(stack)) {
  program(l3Intf);
}

bcm_l3_intf_t BcmLabeledTunnel::getTunnelProperties(bcm_if_t intfId) const {
  bcm_l3_intf_t l3Intf;
  // read interface attribute, over  which to create labeled tunnel
  bcm_l3_intf_t_init(&l3Intf);
  l3Intf.l3a_intf_id = intfId;
  bcm_l3_intf_get(hw_->getUnit(), &l3Intf);
  // clear relevant fields
  l3Intf.l3a_intf_id = INVALID;
  l3Intf.l3a_flags = 0;
  return l3Intf;
}

void BcmLabeledTunnel::program(bcm_if_t l3Intf) {
  auto intfParams = getTunnelProperties(l3Intf);

  auto* wbCache = hw_->getWarmBootCache();
  auto citr = wbCache->findTunnel(intfParams.l3a_vid, stack_);
  if (citr != wbCache->labelStack2TunnelId_end()) {
    labeledTunnel_ = citr->second;
    XLOG(DBG3) << "found tunnel interface:" << str() << " in warmboot cache";
    wbCache->programmed(citr);
    return;
  }

  auto rv = bcm_l3_intf_create(hw_->getUnit(), &intfParams);
  bcmCheckError(
      rv, "failed to create tunnel interface with ", strLabelStack(stack_));
  labeledTunnel_ = intfParams.l3a_intf_id;
  XLOG(DBG3) << "created tunnel interface:" << str();
  setupTunnelLabels();
}

BcmLabeledTunnel::~BcmLabeledTunnel() {
  if (labeledTunnel_ == INVALID) {
    return;
  }
  clearTunnelLabels();
  bcm_l3_intf_t intf;
  bcm_l3_intf_t_init(&intf);
  intf.l3a_intf_id = labeledTunnel_;
  intf.l3a_flags |= BCM_L3_WITH_ID;
  auto rc = bcm_l3_intf_delete(hw_->getUnit(), &intf);
  bcmCheckError(rc, "failed to delete mpls tunnel ", labeledTunnel_);
  XLOG(DBG3) << "deleted mpls tunnel " << str();
}

std::string BcmLabeledTunnel::str() const {
  return folly::to<std::string>(labeledTunnel_, "->", strLabelStack(stack_));
}

void BcmLabeledTunnel::setupTunnelLabels() {
  std::unique_ptr<bcm_mpls_egress_label_t[]> labelStack = nullptr;
  std::optional<BcmQosPolicyHandle> qosMapId;
  auto defaultDataPlaneQosPolicy =
      hw_->getQosPolicyTable()->getDefaultDataPlaneQosPolicyIf();
  if (defaultDataPlaneQosPolicy) {
    qosMapId.emplace(static_cast<int>(
        defaultDataPlaneQosPolicy->getHandle(BcmQosMap::Type::MPLS_EGRESS)));
  }
  int numLabels = stack_.size();
  if (numLabels) {
    labelStack = std::make_unique<bcm_mpls_egress_label_t[]>(numLabels);
    setupLabelStack(&labelStack[0], stack_, qosMapId);
  }
  auto rv = bcm_mpls_tunnel_initiator_set(
      hw_->getUnit(), labeledTunnel_, numLabels, labelStack.get());
  bcmCheckError(
      rv, "failed to set mpls tunnel initiator parameters to ", str());
  XLOG(DBG3) << "setup mpls tunnel " << str();
}

void BcmLabeledTunnel::clearTunnelLabels() {
  auto rv = bcm_mpls_tunnel_initiator_clear(hw_->getUnit(), labeledTunnel_);
  bcmCheckError(
      rv, "failed to clear mpls tunnel initiator parameters for ", str());
  XLOG(DBG3) << "cleared mpls tunnel " << str();
}

} // namespace facebook::fboss
