// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

#include <set>

namespace facebook::fboss {

class BroadcomXgsAsic : public BroadcomAsic {
 public:
  using BroadcomAsic::BroadcomAsic;
  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  std::vector<prbs::PrbsPolynomial> getSupportedPrbsPolynomials()
      const override {
    return {
        prbs::PrbsPolynomial::PRBS7,
        prbs::PrbsPolynomial::PRBS15,
        prbs::PrbsPolynomial::PRBS23,
        prbs::PrbsPolynomial::PRBS31,
        prbs::PrbsPolynomial::PRBS9,
        prbs::PrbsPolynomial::PRBS11,
        prbs::PrbsPolynomial::PRBS58};
  }
};
} // namespace facebook::fboss
