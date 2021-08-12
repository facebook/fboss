/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/ExternalPhyPort.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/platforms/wedge/facebook/elbert/ElbertPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/fuji/FujiPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/minipack/MinipackPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/yamp/YampPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/ExternalPhyPortStatsUtils.h"
#include "fboss/lib/phy/NullPortStats.h"
#include "fboss/lib/phy/facebook/bcm/minipack/MinipackPortStats.h"
#include "fboss/lib/phy/facebook/credo/yamp/YampPortStats.h"

namespace facebook::fboss {
template <typename PlatformT, typename PortStatsT>
void ExternalPhyPort<PlatformT, PortStatsT>::portChanged(
    std::shared_ptr<Port> oldPort,
    std::shared_ptr<Port> newPort,
    WedgePort* platPort) {
  if (!newPort->isEnabled()) {
    // No need to mess with disabled ports
    XLOG(DBG1) << "Skip reprogramming platform port on disabled port: "
               << newPort->getName();
    return;
  }

  auto profileID = newPort->getProfileID();
  if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
    throw FbossError("Found default profile for port ", newPort->getName());
  }

  auto enabling = (!oldPort || !oldPort->isEnabled());
  auto changingSpeed = (!oldPort || profileID != oldPort->getProfileID());
  auto changingPrbsState = oldPort &&
      (oldPort->getGbSystemPrbs() != newPort->getGbSystemPrbs() ||
       oldPort->getGbLinePrbs() != newPort->getGbLinePrbs());
  // TODO(ccpowers): Eventually we should allow qsfp service to alert the
  // wedge agent to transceiver swaps. This method currently doesn't support
  // 100G FR1, since the port won't flap, it will just go down w/ the default
  // settings.
  // Check if the port flapped, in which case we should check if we need to
  // reprogram the phy. (in case the optics was swapped)
  auto statusChanged = (!oldPort || oldPort->isUp() != newPort->isUp());

  if (!enabling && !changingSpeed && !changingPrbsState && !statusChanged) {
    XLOG(DBG1) << "No need to reprogram " << newPort->getName();
    return;
  }

  const auto& platformPortEntry = platPort->getPlatformPortEntry();
  auto portPinConfig = platPort->getPortXphyPinConfig(profileID);
  auto platform = dynamic_cast<PlatformT*>(platPort->getPlatform());
  auto portProfileConfig = platPort->getPortProfileConfig(profileID);

  const auto& chips = platform->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("No DataPlanePhyChips found");
  }

  phy::PhyPortConfig phyPortConfig;
  phyPortConfig.config = phy::ExternalPhyConfig::fromConfigeratorTypes(
      portPinConfig,
      utility::getXphyLinePolaritySwapMap(
          *platformPortEntry.mapping_ref()->pins_ref(), chips));
  phyPortConfig.profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(portProfileConfig);

  // Use PhyInterfaceHandler to access Phy functions
  platform->getPhyInterfaceHandler()->programOnePort(
      phyID_, newPort->getID().t, profileID, phyPortConfig);

  if (changingPrbsState) {
    XLOG(INFO) << "Trying to setPortPrbs for port " << newPort->getID();

    auto setupPortPrbsAndCollection =
        [&](phy::Side side, bool enable, int32_t polynominal) {
          // Use PhyInterfaceHandler to access Phy functions
          platform->getPhyInterfaceHandler()->setPortPrbs(
              phyID_,
              newPort->getID().t,
              profileID,
              phyPortConfig,
              side,
              enable,
              polynominal);
          // Use PhyInterfaceHandler to access Phy functions
          auto laneSpeed = platform->getPhyInterfaceHandler()->getLaneSpeed(
              phyID_, newPort->getID().t, profileID, phyPortConfig, side);

          auto lockedStats = portStats_.wlock();
          if (!lockedStats->has_value()) {
            lockedStats->emplace(newPort->getName());
          }

          const auto& lanes = (side == phy::Side::SYSTEM)
              ? phyPortConfig.config.system.lanes
              : phyPortConfig.config.line.lanes;
          std::vector<LaneID> sideLanes;
          for (const auto& it : lanes) {
            sideLanes.push_back(it.first);
          }
          (*lockedStats)->setupPrbsCollection(side, sideLanes, laneSpeed);
        };

    if (oldPort->getGbSystemPrbs() != newPort->getGbSystemPrbs()) {
      auto newGbSystemPrbsState = newPort->getGbSystemPrbs();
      setupPortPrbsAndCollection(
          phy::Side::SYSTEM,
          *newGbSystemPrbsState.enabled_ref(),
          *newGbSystemPrbsState.polynominal_ref());
    }

    if (oldPort->getGbLinePrbs() != newPort->getGbLinePrbs()) {
      auto newGbLinePrbsState = newPort->getGbLinePrbs();
      setupPortPrbsAndCollection(
          phy::Side::LINE,
          *newGbLinePrbsState.enabled_ref(),
          *newGbLinePrbsState.polynominal_ref());
    }
  }

  xphyConfig_ = phyPortConfig;
}

template class ExternalPhyPort<MinipackPlatform, MinipackPortStats>;
template class ExternalPhyPort<YampPlatform, YampPortStats>;
template class ExternalPhyPort<FujiPlatform, NullPortStats>;
template class ExternalPhyPort<ElbertPlatform, NullPortStats>;
} // namespace facebook::fboss
