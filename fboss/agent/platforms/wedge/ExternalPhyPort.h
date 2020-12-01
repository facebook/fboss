/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <optional>

#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/phy/ExternalPhy.h"

namespace facebook::fboss {

template <typename PlatformT, typename PortStatsT>
class ExternalPhyPort {
 public:
  ExternalPhyPort() {}

  void portChanged(
      std::shared_ptr<Port> oldPort,
      std::shared_ptr<Port> newPort,
      WedgePort* platPort);

 protected:
  folly::Synchronized<std::optional<PortStatsT>> portStats_;
  std::optional<phy::PhyPortConfig> xphyConfig_;
  int phyID_{-1};

 private:
  // Forbidden copy constructor and assignment operator
  ExternalPhyPort(ExternalPhyPort const&) = delete;
  ExternalPhyPort& operator=(ExternalPhyPort const&) = delete;
};

} // namespace facebook::fboss
