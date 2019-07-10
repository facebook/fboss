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

 private:
  void prepareEgressObject(
      opennsl_if_t intfId,
      opennsl_port_t port,
      const folly::Optional<folly::MacAddress>& mac,
      RouteForwardAction action,
      opennsl_l3_egress_t* eObj) const override;

 private:
  BcmWarmBootCache::EgressId2EgressCitr findEgress(
      opennsl_vrf_t vrf,
      opennsl_if_t intfId,
      const folly::IPAddress& ip) const override;

  std::shared_ptr<BcmLabeledTunnel> tunnel_;
};

} // namespace fboss
} // namespace facebook
