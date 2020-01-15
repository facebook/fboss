// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmEgress.h"

extern "C" {
#include <bcm/types.h>
}

namespace facebook::fboss {

class BcmLabeledEgress : public BcmEgress {
 public:
  BcmLabeledEgress(const BcmSwitchIf* hw, bcm_mpls_label_t label)
      : BcmEgress(hw), label_(label) {}

  bool hasLabel() const override {
    return true;
  }

  bcm_mpls_label_t getLabel() const override {
    return label_;
  }

 protected:
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

  bcm_mpls_label_t label_;
};

} // namespace facebook::fboss
