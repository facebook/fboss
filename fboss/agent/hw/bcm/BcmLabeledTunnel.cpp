// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

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

auto constexpr kStack = "stack";
auto constexpr kIntf = "labeledTunnel";
} // namespace
namespace facebook {
namespace fboss {

BcmLabeledTunnel::BcmLabeledTunnel(
    BcmSwitch* hw,
    opennsl_if_t l3Intf,
    LabelForwardingAction::LabelStack stack)
    : hw_(hw), stack_(std::move(stack)) {
  program(l3Intf);
}

opennsl_l3_intf_t BcmLabeledTunnel::getTunnelProperties(
    opennsl_if_t intfId) const {
  opennsl_l3_intf_t l3Intf;
  // read interface attribute, over  which to create labeled tunnel
  opennsl_l3_intf_t_init(&l3Intf);
  l3Intf.l3a_intf_id = intfId;
  opennsl_l3_intf_get(hw_->getUnit(), &l3Intf);
  // clear relevant fields
  l3Intf.l3a_intf_id = INVALID;
  l3Intf.l3a_flags = 0;
  return l3Intf;
}

void BcmLabeledTunnel::program(opennsl_if_t l3Intf) {
  auto intfParams = getTunnelProperties(l3Intf);
  CHECK_GT(stack_.size(), 1);

  auto rv = opennsl_l3_intf_create(hw_->getUnit(), &intfParams);
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
  opennsl_l3_intf_t intf;
  opennsl_l3_intf_t_init(&intf);
  intf.l3a_intf_id = labeledTunnel_;
  intf.l3a_flags |= OPENNSL_L3_WITH_ID;
  auto rc = opennsl_l3_intf_delete(hw_->getUnit(), &intf);
  bcmCheckError(rc, "failed to delete mpls tunnel ", labeledTunnel_);
  XLOG(DBG3) << "deleted mpls tunnel " << str();
}

std::string BcmLabeledTunnel::str() const {
  return folly::to<std::string>(labeledTunnel_, "->", strLabelStack(stack_));
}

folly::dynamic BcmLabeledTunnel::toFollyDynamic() const {
  folly::dynamic tunnel = folly::dynamic::object;
  tunnel[kIntf] = labeledTunnel_;
  folly::dynamic labels = folly::dynamic::array;
  for (const auto& label : stack_) {
    labels.push_back(label);
  }
  tunnel[kStack] = std::move(labels);
  return tunnel;
}
} // namespace fboss
} // namespace facebook
