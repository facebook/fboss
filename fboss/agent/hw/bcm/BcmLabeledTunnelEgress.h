// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"
#include "fboss/agent/state/LabelForwardingAction.h"

extern "C" {
#include <bcm/types.h>
}

namespace facebook::fboss {

class BcmLabeledTunnel;

class BcmLabeledTunnelEgress : public BcmLabeledEgress {
 public:
  BcmLabeledTunnelEgress(
      const BcmSwitchIf* hw,
      bcm_mpls_label_t label,
      bcm_if_t interface,
      const LabelForwardingAction::LabelStack& labelStack);
  ~BcmLabeledTunnelEgress() override;
  BcmLabeledTunnel* getTunnel() const {
    return tunnel_.get();
  }

 private:
  void prepareEgressObject(
      bcm_if_t intfId,
      bcm_port_t port,
      const std::optional<folly::MacAddress>& mac,
      RouteForwardAction action,
      bcm_l3_egress_t* eObj) const override;

 private:
  BcmWarmBootCache::EgressId2EgressCitr findEgress(
      bcm_vrf_t vrf,
      bcm_if_t intfId,
      const folly::IPAddress& ip) const override;

  std::shared_ptr<BcmLabeledTunnel> tunnel_;
};

} // namespace facebook::fboss
