// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/LabelForwardingEntry.h"

namespace facebook::fboss {

class BcmSwitch;
class BcmLabelSwitchAction;

class BcmLabelMap {
 public:
  using BcmLabel = uint32_t;
  BcmLabelMap(BcmSwitch* hw);
  ~BcmLabelMap();
  void processAddedLabelSwitchAction(
      BcmLabel topLabel,
      const LabelNextHopEntry& nexthops);
  void processRemovedLabelSwitchAction(BcmLabel topLabel);
  void processChangedLabelSwitchAction(
      BcmLabel topLabel,
      const LabelNextHopEntry& nexthops);

 private:
  using BcmLabelSwitchActionMap = boost::container::
      flat_map<BcmLabel, std::unique_ptr<BcmLabelSwitchAction>>;

  BcmSwitch* hw_;
  BcmLabelSwitchActionMap labelMap_;
};

} // namespace facebook::fboss
