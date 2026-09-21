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
  int getMidPriCpuQueueId() const override;
  int getHiPriCpuQueueId() const override;
};
} // namespace facebook::fboss
