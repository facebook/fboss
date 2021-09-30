/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

/*
 * SaiPortManager::addPort
 *
 * This function takes the Port object with the given port settings and based on
 * that it creates sai port. It calls attributesFromSwPort() to get the PortApi
 * Attributes for the port and then creates Sai port using PortStore.setObject()
 * call. This function creates system port, line port and then port connector.
 * It stores all port objects in SaiPortHandle.
 */
PortSaiId SaiPortManager::addPort(const std::shared_ptr<Port>& swPort) {
  SaiPortHandle* portHandle = getPortHandle(swPort->getID());
  if (portHandle) {
    // TODO(ccpowers): We should throw an error here once we have a
    // changePort() implementation for phys: T97090413.
    XLOG(WARNING) << "Adding port which already exists: " << swPort->getID()
                  << " SAI id: " << portHandle->port->adapterKey();
  }

  // Create the system port first
  SaiPortTraits::CreateAttributes attributes =
      attributesFromSwPort(swPort, false);
  SaiPortTraits::AdapterHostKey sysPortKey{
      GET_ATTR(Port, HwLaneList, attributes)};
  auto& portStore = saiStore_->get<SaiPortTraits>();
  auto saiSysPort =
      portStore.setObject(sysPortKey, attributes, swPort->getID());
  XLOG(DBG3) << "Created sysport " << saiSysPort->adapterKey();

  // Create line side port
  attributes = attributesFromSwPort(swPort, true);
  SaiPortTraits::AdapterHostKey linePortKey{
      GET_ATTR(Port, HwLaneList, attributes)};
  auto saiLinePort =
      portStore.setObject(linePortKey, attributes, swPort->getID());
  XLOG(DBG3) << "Created lineport " << saiLinePort->adapterKey();

  // Create the port connector
  SaiPortConnectorTraits::CreateAttributes portConnAttr{
      saiLinePort->adapterKey(), saiSysPort->adapterKey()};
  auto portConnKey = portConnAttr;

  auto& portConnStore = saiStore_->get<SaiPortConnectorTraits>();
  auto saiPortConn = portConnStore.setObject(portConnKey, portConnAttr);
  XLOG(DBG3) << "Created port connector " << saiPortConn->adapterKey();

  // Make admin state of sysport and lineport up after the connector  is created
  saiSysPort->setOptionalAttribute(SaiPortTraits::Attributes::AdminState{true});
  saiLinePort->setOptionalAttribute(
      SaiPortTraits::Attributes::AdminState{true});

  XLOG(DBG3) << folly::sformat(
      "Port admin state of lineport {:d} and sysport {:d} made up",
      static_cast<uint64_t>(saiLinePort->adapterKey()),
      static_cast<uint64_t>(saiSysPort->adapterKey()));

  // Record the line port handle and port connector handles
  auto handle = std::make_unique<SaiPortHandle>();
  handle->port = saiLinePort;
  handle->sysPort = saiSysPort;
  handle->connector = saiPortConn;
  handles_.emplace(swPort->getID(), std::move(handle));

  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());
  return saiLinePort->adapterKey();
}

void SaiPortManager::changePort(
    const std::shared_ptr<Port>& /*oldPort*/,
    const std::shared_ptr<Port>& newPort) {
  // TODO - properly handle attribute update, rather than treting
  // all changes as a port addition
  addPort(newPort);
}

/*
 * SaiPortManager::attributesFromSwPort
 *
 * This function takes the Port object with the given port settings and returns
 * the port create attributes. This function uses the hardware lanes based on
 * the lineSide parameter. This function is called for system port and the line
 * port for creating a port connector
 */
SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort,
    bool lineSide) const {
  // Get the user specified swPortId and portProfileId
  auto profileId = swPort->getProfileID();
  auto portId = swPort->getID();
  auto enabled = swPort->isEnabled();
  auto platPort = platform_->getPort(portId);

  const auto& platformPortEntry = platPort->getPlatformPortEntry();
  const auto& portPinConfig = platPort->getPortXphyPinConfig(profileId);
  const auto& portProfileConfig = platPort->getPortProfileConfig(profileId);

  const auto& chips = platform_->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("No DataPlanePhyChips found");
  }

  const auto& config = phy::ExternalPhyConfig::fromConfigeratorTypes(
      portPinConfig,
      utility::getXphyLinePolaritySwapMap(
          *platformPortEntry.mapping_ref()->pins_ref(), chips));
  const auto& profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(portProfileConfig);

  auto speed = profile.speed;
  auto fecMode = lineSide ? profile.line.fec_ref() : profile.system.fec_ref();

  std::vector<uint32_t> laneList;
  const auto& lanes = lineSide ? config.line.lanes : config.system.lanes;
  for (auto& lane : lanes) {
    laneList.push_back(lane.first);
  }

  return SaiPortTraits::CreateAttributes {
    laneList, static_cast<uint32_t>(speed), enabled,
        utility::getSaiPortFecMode(*fecMode), std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt,
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
        std::nullopt, std::nullopt,
#endif
        std::nullopt, std::nullopt, std::nullopt, std::nullopt
  };
}
} // namespace facebook::fboss
