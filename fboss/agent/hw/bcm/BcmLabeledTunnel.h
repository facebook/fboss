// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include "fboss/agent/state/RouteNextHop.h"

#include <folly/dynamic.h>

namespace facebook::fboss {

class BcmSwitch;

class BcmLabeledTunnel {
 public:
  enum : bcm_if_t {
    INVALID = -1,
  };

  BcmLabeledTunnel(
      BcmSwitch* hw,
      bcm_if_t intf,
      LabelForwardingAction::LabelStack stack);
  ~BcmLabeledTunnel();
  std::string str() const;
  bcm_if_t getTunnelInterface() const {
    return labeledTunnel_;
  }
  const LabelForwardingAction::LabelStack& getTunnelStack() {
    return stack_;
  }

 private:
  bcm_l3_intf_t getTunnelProperties(bcm_if_t intf) const;
  void program(bcm_if_t intf);
  void setupTunnelLabels();
  void clearTunnelLabels();

  BcmSwitch* hw_;
  LabelForwardingAction::LabelStack stack_;
  bcm_if_t labeledTunnel_{INVALID};
};

} // namespace facebook::fboss
