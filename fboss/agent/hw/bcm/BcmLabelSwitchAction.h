// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/mpls.h>
}
#include "fboss/agent/state/LabelForwardingEntry.h"

namespace facebook::fboss {

class BcmSwitch;
class BcmMultiPathNextHop;

class BcmLabelSwitchAction {
 public:
  BcmLabelSwitchAction(
      BcmSwitch* hw,
      bcm_mpls_label_t topLabel,
      const LabelNextHopEntry& entry);

  ~BcmLabelSwitchAction();

 private:
  bcm_if_t getEgress(const LabelNextHopEntry& entry);
  bcm_if_t getLabeledEgress(const LabelNextHopSet& nexthops);
  bcm_if_t getUnlabeledEgress(const LabelNextHopSet& nexthops);

  BcmLabelSwitchAction(const BcmLabelSwitchAction&) = delete;
  BcmLabelSwitchAction(BcmLabelSwitchAction&&) = delete;
  BcmLabelSwitchAction& operator=(const BcmLabelSwitchAction&) = delete;
  BcmLabelSwitchAction& operator=(BcmLabelSwitchAction&&) = delete;

  int unit_;
  // reference to resolved next hop(s)
  std::shared_ptr<BcmMultiPathNextHop> nexthop_;
  // mpls tunnel switch action
  bcm_mpls_tunnel_switch_t action_;
};

} // namespace facebook::fboss
