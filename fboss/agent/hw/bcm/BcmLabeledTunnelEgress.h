// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"
#include "fboss/agent/state/LabelForwardingAction.h"

extern "C" {
#include <opennsl/types.h>
}

namespace facebook {
namespace fboss {

class BcmLabeledTunnel;

class BcmLabeledTunnelEgress : public BcmLabeledEgress {
 public:
  BcmLabeledTunnelEgress(
      const BcmSwitchIf* hw,
      opennsl_mpls_label_t label,
      opennsl_if_t interface,
      const LabelForwardingAction::LabelStack& labelStack);
  ~BcmLabeledTunnelEgress() override;
  BcmLabeledTunnel* getTunnel() const {
    return tunnel_.get();
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const override;

 private:
  int createEgress(int unit, uint32_t flags, opennsl_l3_egress_t* egr) override;

  std::shared_ptr<BcmLabeledTunnel> tunnel_;
};

} // namespace fboss
} // namespace facebook
