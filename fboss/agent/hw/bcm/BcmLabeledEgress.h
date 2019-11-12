// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmEgress.h"

extern "C" {
#include <opennsl/types.h>
}

namespace facebook {
namespace fboss {

class BcmLabeledEgress : public BcmEgress {
 public:
  BcmLabeledEgress(const BcmSwitchIf* hw, opennsl_mpls_label_t label)
      : BcmEgress(hw), label_(label) {}

  bool hasLabel() const override {
    return true;
  }

  opennsl_mpls_label_t getLabel() const override {
    return label_;
  }

 protected:
  void prepareEgressObject(
      opennsl_if_t intfId,
      opennsl_port_t port,
      const std::optional<folly::MacAddress>& mac,
      RouteForwardAction action,
      opennsl_l3_egress_t* eObj) const override;

 private:
  BcmWarmBootCache::EgressId2EgressCitr findEgress(
      opennsl_vrf_t vrf,
      opennsl_if_t intfId,
      const folly::IPAddress& ip) const override;

  opennsl_mpls_label_t label_;
};

} // namespace fboss
} // namespace facebook
