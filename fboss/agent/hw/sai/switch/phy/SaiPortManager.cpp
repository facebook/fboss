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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

void SaiPortManager::fillInSupportedStats(PortID portId) {
  port2SupportedStats_.insert({portId, {}});
}
/*
 * SaiPortManager::addPort
 *
 * This function takes the Port object with the given port settings and based on
 * that it creates sai port. It calls attributesFromSwPort() to get the PortApi
 * Attributes for the port and then creates Sai port using PortStore.setObject()
 * call. This function creates system port, line port and then port connector.
 * It stores all port objects in SaiPortHandle.
 */
PortSaiId SaiPortManager::addPortImpl(const std::shared_ptr<Port>& swPort) {
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

  // Record the line port handle and port connector handles
  auto handle = std::make_unique<SaiPortHandle>();
  handle->sysPort = saiSysPort;

  // Create line side port
  attributes = attributesFromSwPort(swPort, true);
  SaiPortTraits::AdapterHostKey linePortKey{
      GET_ATTR(Port, HwLaneList, attributes)};
  auto saiLinePort =
      portStore.setObject(linePortKey, attributes, swPort->getID());
  XLOG(DBG3) << "Created lineport " << saiLinePort->adapterKey();
  handle->port = saiLinePort;

  // Program System and Line side Serdes
  programSerdes(saiLinePort, swPort, handle.get());

  // Create the port connector
  SaiPortConnectorTraits::CreateAttributes portConnAttr{
      saiLinePort->adapterKey(), saiSysPort->adapterKey()};
  auto portConnKey = portConnAttr;

  auto& portConnStore = saiStore_->get<SaiPortConnectorTraits>();
  auto saiPortConn = portConnStore.setObject(portConnKey, portConnAttr);
  XLOG(DBG3) << "Created port connector " << saiPortConn->adapterKey();

  handle->connector = saiPortConn;
  handles_.emplace(swPort->getID(), std::move(handle));

  // Make admin state of sysport and lineport up after the connector  is created
  saiSysPort->setOptionalAttribute(SaiPortTraits::Attributes::AdminState{true});
  saiLinePort->setOptionalAttribute(
      SaiPortTraits::Attributes::AdminState{true});

  XLOG(DBG3) << folly::sformat(
      "Port admin state of lineport {:d} and sysport {:d} made up",
      static_cast<uint64_t>(saiLinePort->adapterKey()),
      static_cast<uint64_t>(saiSysPort->adapterKey()));

  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());
  programMacsec(nullptr, swPort);
  return saiLinePort->adapterKey();
}

void SaiPortManager::loadPortQueuesForAddedPort(
    const std::shared_ptr<Port>& /*swPort*/) {}
void SaiPortManager::loadPortQueuesForChangedPort(
    const std::shared_ptr<Port>& /*oldPort*/,
    const std::shared_ptr<Port>& /*newPort*/) {}

void SaiPortManager::changePortImpl(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  auto nonMacsecFieldsChange = [](const auto& l, const auto& r) {
    if (l.getTxSak() == r.getTxSak() && l.getRxSaksMap() == r.getRxSaksMap() &&
        l.getMacsecDesired() == r.getMacsecDesired() &&
        l.getDropUnencrypted() == r.getDropUnencrypted()) {
      // We got a port change, while MACSEC fields did not change
      return true;
    }
    return false;
  };

  // Only add/reprogram port if something besides MACSEC changed
  if (nonMacsecFieldsChange(*oldPort, *newPort)) {
    // Clear MACSEC, we will program that just after addPort
    auto portSansMacsec = newPort->clone();
    portSansMacsec->setTxSak(std::nullopt);
    portSansMacsec->setRxSaksMap({});
    addPort(portSansMacsec);
  }
  programMacsec(oldPort, newPort);
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
    bool lineSide,
    bool /* basicAttributeOnly */) const {
  // Get the user specified swPortId and portProfileId
  auto speed = swPort->getSpeed();
  auto portId = swPort->getID();
  auto enabled = swPort->isEnabled();

  // Now use profileConfig from SW port as the source of truth
  if (lineSide && !swPort->getLineProfileConfig()) {
    throw FbossError(
        "Invalid sw Port, missing line profileConfig for port:",
        swPort->getID(),
        ", profile:",
        apache::thrift::util::enumNameSafe(swPort->getProfileID()));
  }
  std::optional<SaiPortTraits::Attributes::FecMode> fecMode;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::FEC)) {
    auto phyFecMode = lineSide ? *swPort->getLineProfileConfig()->fec()
                               : *swPort->getProfileConfig().fec();
    fecMode = utility::getSaiPortFecMode(phyFecMode);
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  std::optional<SaiPortTraits::Attributes::UseExtendedFec> useExtendedFec(
      std::nullopt);
  std::optional<SaiPortTraits::Attributes::ExtendedFecMode> extendedFecMode(
      std::nullopt);
  if (platform_->getAsic()->isSupported(HwAsic::Feature::EXTENDED_FEC)) {
    auto phyFecMode = lineSide ? *swPort->getLineProfileConfig()->fec()
                               : *swPort->getProfileConfig().fec();
    useExtendedFec = true;
    extendedFecMode = utility::getSaiPortExtendedFecMode(phyFecMode);
    // Once extended FEC is defined then undefine the regular FEC
    fecMode = std::nullopt;
  }
#endif

  std::optional<SaiPortTraits::Attributes::InterfaceType> intfType(
      std::nullopt);

  if (platform_->getAsic()->isSupported(HwAsic::Feature::PORT_INTERFACE_TYPE) &&
      platform_->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_SANDIA_PHY) {
    if (!swPort->getProfileConfig().medium() ||
        !swPort->getLineProfileConfig()->medium()) {
      throw FbossError(
          "Missing medium info in profile ",
          apache::thrift::util::enumNameSafe(swPort->getProfileID()));
    }
    auto transmitterTech = lineSide ? *swPort->getLineProfileConfig()->medium()
                                    : *swPort->getProfileConfig().medium();

    if (auto saiInterfaceType =
            platform_->getInterfaceType(transmitterTech, speed)) {
      intfType = saiInterfaceType.value();
    }
  }

  // Now use pinConfigs from SW port as the source of truth
  // TODO: Support programming dynamic tx_settings
  std::vector<uint32_t> laneList;
  if (lineSide && !swPort->getLinePinConfigs()) {
    throw FbossError(
        "Invalid sw Port, missing line pinConfigs for port:",
        swPort->getID(),
        ", profile:",
        apache::thrift::util::enumNameSafe(swPort->getProfileID()));
  }
  const auto& pinCfgs =
      lineSide ? *swPort->getLinePinConfigs() : swPort->getPinConfigs();
  for (const auto& pinCfg : pinCfgs) {
    laneList.push_back(*pinCfg.id()->lane());
  }

  std::string dbgOutput;
  dbgOutput.append(folly::sformat(
      "Attributes for creating port {:d}, Side {:s}, Lanes: ",
      static_cast<int>(portId),
      (lineSide ? "Line" : "System")));
  for (auto lane : laneList) {
    dbgOutput.append(folly::sformat("{:d} ", lane));
  }

  dbgOutput.append(folly::sformat(
      " Speed {:d} Enabled {:s} Fec {:s} ",
      static_cast<int>(speed),
      (enabled ? "True" : "False"),
      (fecMode ? folly::to<std::string>(fecMode->value()) : "null")));
  if (intfType.has_value()) {
    dbgOutput.append(folly::sformat(
        " Interface Type {:d}", static_cast<int>(intfType.value().value())));
  }
  XLOG(DBG2) << dbgOutput;

  return SaiPortTraits::CreateAttributes{
      laneList,       static_cast<uint32_t>(speed),
      enabled,        fecMode,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      useExtendedFec, extendedFecMode,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
      std::nullopt, // Port Fabric Isolate
#endif
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      intfType,       std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
      std::nullopt,   std::nullopt,
#if !defined(TAJO_SDK)
      std::nullopt,   std::nullopt,
#endif
      std::nullopt,   std::nullopt,
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
      std::nullopt,
#endif
      std::nullopt, // Link Training Enable
      std::nullopt, // FDR Enable
      std::nullopt, // Rx Squelch Enable
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      std::nullopt, // PFC Deadlock Detection Interval
      std::nullopt, // PFC Deadlock Recovery Interval
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      std::nullopt, // ARS enable
      std::nullopt, // ARS scaling factor
      std::nullopt, // ARS port load past weight
      std::nullopt, // ARS port load future weight
#endif
      std::nullopt, // Reachability Group
  };
}

void SaiPortManager::enableAfeAdaptiveMode(PortID /*portID*/) {}

/*
 * programSerdes
 *
 * This function programs the Serdes for the PHY port. It programs 2 serdes
 * - one for lien side port and other one for system side port. The Serdes
 * parameters are fetched from the SI parameters put in the platformMapping
 * file
 */
void SaiPortManager::programSerdes(
    std::shared_ptr<SaiPort> saiLinePort,
    std::shared_ptr<Port> swPort,
    SaiPortHandle* portHandle) {
  if (!platform_->isSerdesApiSupported() ||
      !(platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET))) {
    return;
  }

  std::shared_ptr<SaiPort> saiSysPort = portHandle->sysPort;

  // Create the Line side Serdes
  SaiPortSerdesTraits::AdapterHostKey lineSerdesKey{saiLinePort->adapterKey()};
  auto& store = saiStore_->get<SaiPortSerdesTraits>();
  // check if serdes object already exists for given port
  std::shared_ptr<SaiPortSerdes> lineSerdes = store.get(lineSerdesKey);

  if (lineSerdes) {
    // If the Serdes exists then reset it
    lineSerdes.reset();
  }

  // The line side pin config is optional parameter but for PHY it must be
  // provided
  if (!swPort->getLinePinConfigs().has_value()) {
    throw FbossError("Line side pin config not found");
  }

  SaiPortSerdesTraits::CreateAttributes serdesAttributes =
      serdesAttributesFromSwPinConfigs(
          saiLinePort->adapterKey(),
          swPort->getLinePinConfigs().value(),
          lineSerdes);

  // create Line port Serdes
  portHandle->serdes = store.setObject(lineSerdesKey, serdesAttributes);

  // Create the system side Serdes now
  SaiPortSerdesTraits::AdapterHostKey sysSerdesKey{saiSysPort->adapterKey()};
  // check if serdes object already exists for given port
  std::shared_ptr<SaiPortSerdes> sysSerdes = store.get(sysSerdesKey);

  if (sysSerdes) {
    // If the Serdes exists then reset it
    sysSerdes.reset();
  }

  serdesAttributes = serdesAttributesFromSwPinConfigs(
      saiSysPort->adapterKey(), swPort->getPinConfigs(), sysSerdes);

  // create System port serdes
  portHandle->sysSerdes = store.setObject(sysSerdesKey, serdesAttributes);
}

/*
 * serdesAttributesFromSwPinConfigs
 *
 * This function returns the list of attributes for Creating a serdes. The
 * input is taken from list of pin config which could be system side PHY pin
 * config or the line side PHY pin config
 */
SaiPortSerdesTraits::CreateAttributes
SaiPortManager::serdesAttributesFromSwPinConfigs(
    PortSaiId portSaiId,
    const std::vector<phy::PinConfig>& pinConfigs,
    const std::shared_ptr<SaiPortSerdes>& /* serdes */) {
  SaiPortSerdesTraits::CreateAttributes attrs;

  SaiPortSerdesTraits::Attributes::TxFirPre1::ValueType txPre1;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  SaiPortSerdesTraits::Attributes::TxFirPre1::ValueType txPre2;
#endif
  SaiPortSerdesTraits::Attributes::TxFirMain::ValueType txMain;
  SaiPortSerdesTraits::Attributes::TxFirPost1::ValueType txPost1;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  SaiPortSerdesTraits::Attributes::TxFirPost1::ValueType txPost2;
  SaiPortSerdesTraits::Attributes::TxFirPost1::ValueType txPost3;
#endif

  // Now use pinConfigs from SW port as the source of truth
  auto numExpectedTxLanes = 0;
  for (const auto& pinConfig : pinConfigs) {
    if (auto tx = pinConfig.tx()) {
      ++numExpectedTxLanes;
      txPre1.push_back(*tx->pre());
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      txPre2.push_back(*tx->pre2());
#endif
      txMain.push_back(*tx->main());
      txPost1.push_back(*tx->post());
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      txPost2.push_back(*tx->post2());
      txPost3.push_back(*tx->post3());
#endif
    }
  }

  auto setTxAttr = [](auto& attrs, auto type, const auto& val) {
    auto& attr = std::get<std::optional<std::decay_t<decltype(type)>>>(attrs);
    if (!val.empty()) {
      attr = val;
    }
  };

  std::get<SaiPortSerdesTraits::Attributes::PortId>(attrs) =
      static_cast<sai_object_id_t>(portSaiId);
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPre1{}, txPre1);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPre2{}, txPre2);
#endif
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost1{}, txPost1);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost2{}, txPost2);
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost3{}, txPost3);
#endif
  setTxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirMain{}, txMain);

  std::string dbgOutput;
  dbgOutput.append(folly::sformat(
      "Attributes for creating port {} Serdes (per lane): ",
      static_cast<uint64_t>(portSaiId)));

  for (auto lane = 0; lane < numExpectedTxLanes; lane++) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    dbgOutput.append(folly::sformat(
        "[Pre1 {:d} Pre2 {:d} Main {:d} Post1 {:d} Post2 {:d} Post3 {:d}], ",
        (lane < txPre1.size() ? txPre1[lane] : static_cast<unsigned int>(-1)),
        (lane < txPre2.size() ? txPre2[lane] : static_cast<unsigned int>(-1)),
        (lane < txMain.size() ? txMain[lane] : static_cast<unsigned int>(-1)),
        (lane < txPost1.size() ? txPost1[lane] : static_cast<unsigned int>(-1)),
        (lane < txPost2.size() ? txPost2[lane] : static_cast<unsigned int>(-1)),
        (lane < txPost3.size() ? txPost3[lane]
                               : static_cast<unsigned int>(-1))));
#else
    dbgOutput.append(folly::sformat(
        "[Pre1 {:d} Main {:d} Post1 {:d}], ",
        (lane < txPre1.size() ? txPre1[lane] : static_cast<unsigned int>(-1)),
        (lane < txMain.size() ? txMain[lane] : static_cast<unsigned int>(-1)),
        (lane < txPost1.size() ? txPost1[lane]
                               : static_cast<unsigned int>(-1))));
#endif
  }
  XLOG(INFO) << dbgOutput;

  return attrs;
}
} // namespace facebook::fboss
