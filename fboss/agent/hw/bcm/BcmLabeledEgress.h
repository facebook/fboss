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
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const override {
    CHECK(0); // TODO(pshaikh): this must not be called, remove this
    return folly::dynamic::object;
  }

  bool hasLabel() const override {
    return true;
  }

  opennsl_mpls_label_t getLabel() const override {
    return label_;
  }

 private:
  void setLabel(opennsl_l3_egress_t* egress) const override;
  int createEgress(int unit, uint32_t flags, opennsl_l3_egress_t* egr) override;

  opennsl_mpls_label_t label_;
};

} // namespace fboss
} // namespace facebook
