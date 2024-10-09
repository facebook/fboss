// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/FileUtil.h>
#include <folly/json/DynamicConverter.h>
#include <folly/json/json.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace std::chrono;

// allow us to configure the qsfp_service dir so that the qsfp cold boot test
// can run concurrently with itself
DEFINE_string(
    qsfp_service_volatile_dir,
    "/dev/shm/fboss/qsfp_service",
    "Path to the directory in which we store the qsfp_service's cold boot flag");

DEFINE_bool(
    can_qsfp_service_warm_boot,
    true,
    "Enable/disable warm boot functionality for qsfp_service");

DEFINE_int32(
    state_machine_update_thread_heartbeat_ms,
    20000,
    "State machine update thread's heartbeat interval (ms)");

DEFINE_bool(
    firmware_upgrade_supported,
    false,
    "Set to true to enable firmware upgrade support");

DEFINE_int32(
    max_concurrent_evb_fw_upgrade,
    1,
    "How many transceivers sharing the same evb to schedule a firmware upgrade on at a time");

DEFINE_int32(
    firmware_upgrade_time_limit,
    1080,
    "Maximum time limit (in seconds) for a firmware upgrade test. Exceeding this limit will increment an fb303 counter that keeps track of slow firmware upgrade tests.");

DEFINE_bool(
    enable_tcvr_validation,
    false,
    "Enable transceiver validation feature in qsfp_service");

DEFINE_bool(
    firmware_upgrade_on_coldboot,
    false,
    "Set to true to automatically upgrade firmware on coldboot");

DEFINE_bool(
    firmware_upgrade_on_link_down,
    false,
    "Set to true to automatically upgrade firmware when link goes down");

DEFINE_bool(
    firmware_upgrade_on_tcvr_insert,
    false,
    "Set to true to automatically upgrade firmware when a transceiver is inserted");

namespace {
constexpr auto kForceColdBootFileName = "cold_boot_once_qsfp_service";
constexpr auto kWarmBootFlag = "can_warm_boot";
constexpr auto kWarmbootStateFileName = "qsfp_service_state";
constexpr auto kPhyStateKey = "phy";
constexpr auto kAgentConfigAppliedInfoStateKey = "agentConfigAppliedInfo";
constexpr auto kAgentConfigLastAppliedInMsKey = "agentConfigLastAppliedInMs";
constexpr auto kAgentConfigLastColdbootAppliedInMsKey =
    "agentConfigLastColdbootAppliedInMs";
static constexpr auto kStateMachineThreadHeartbeatMissed =
    "state_machine_thread_heartbeat_missed";
constexpr int kSecAfterModuleOutOfReset = 2;

std::map<int, facebook::fboss::NpuPortStatus> getNpuPortStatus(
    const std::map<int32_t, facebook::fboss::PortStatus>& portStatus) {
  std::map<int, facebook::fboss::NpuPortStatus> npuPortStatus;
  for (const auto& [portId, status] : portStatus) {
    facebook::fboss::NpuPortStatus npuStatus;
    npuStatus.portId = portId;
    npuStatus.operState = status.get_up();
    npuStatus.portEnabled = status.get_enabled();
    npuStatus.profileID = status.get_profileID();
    npuPortStatus.emplace(portId, npuStatus);
  }
  return npuPortStatus;
}
} // namespace

namespace facebook::fboss {

TransceiverManager::TransceiverManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping)
    : qsfpPlatApi_(std::move(api)),
      platformMapping_(std::move(platformMapping)),
      stateMachines_(setupTransceiverToStateMachineHelper()),
      tcvrToPortInfo_(setupTransceiverToPortInfo()) {
  // Cache the static mapping based on platformMapping_
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  const auto& chips = platformMapping_->getChips();
  for (const auto& [portIDInt, platformPort] : platformPorts) {
    PortID portID = PortID(portIDInt);
    const auto& portName = *platformPort.mapping()->name();
    portNameToPortID_.insert(PortNameIdMap::value_type(portName, portID));
    SwPortInfo portInfo;
    portInfo.name = portName;
    portInfo.tcvrID = utility::getTransceiverId(platformPort, chips);
    portToSwPortInfo_.emplace(portID, std::move(portInfo));
  }
  try {
    fwStorage_ =
        std::make_unique<FbossFwStorage>(FbossFwStorage::initStorage());
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Couldn't create FbossFwStorage instance: "
              << folly::exceptionStr(ex);
  }

  initPortToModuleMap();

  // Initialize the resetFunctionMap_ with proper function
  // per reset type/action. map is better than multiple switch
  // statements when we support more reset types.
  resetFunctionMap_[std::make_pair(
      ResetType::HARD_RESET, ResetAction::RESET_THEN_CLEAR)] =
      &TransceiverManager::triggerQsfpHardReset;
  resetFunctionMap_[std::make_pair(ResetType::HARD_RESET, ResetAction::RESET)] =
      &TransceiverManager::holdTransceiverReset;
  resetFunctionMap_[std::make_pair(
      ResetType::HARD_RESET, ResetAction::CLEAR_RESET)] =
      &TransceiverManager::releaseTransceiverReset;
}

TransceiverManager::~TransceiverManager() {
  // Make sure if gracefulExit() is not called, we will still stop the threads
  if (!isExiting_) {
    setGracefulExitingFlag();
    stopThreads();
  }
}

void TransceiverManager::initPortToModuleMap() {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  for (const auto& it : platformPorts) {
    auto port = it.second;
    // Get the transceiver id based on the port info from platform mapping.
    auto portId = *port.mapping()->id();
    auto transceiverId = getTransceiverID(PortID(portId));
    if (!transceiverId) {
      XLOG(ERR) << "Did not find transceiver id for port id " << portId;
      continue;
    }
    // Add the port to the transceiver indexed port group.
    auto portGroupIt = portGroupMap_.find(transceiverId.value());
    if (portGroupIt == portGroupMap_.end()) {
      portGroupMap_[transceiverId.value()] =
          std::set<cfg::PlatformPortEntry>{port};
    } else {
      portGroupIt->second.insert(port);
    }
    std::string portName = *port.mapping()->name();
    portNameToModule_[portName] = transceiverId.value();
    XLOG(DBG2) << "Added port " << portName << " with portId " << portId
               << " to transceiver " << transceiverId.value();
  }
}

void TransceiverManager::initTcvrValidator() {
  auto tcvrValConfig = qsfpConfig_->thrift.transceiverValidationConfig();

  if (FLAGS_enable_tcvr_validation && tcvrValConfig.has_value()) {
    tcvrValidator_ =
        std::make_unique<TransceiverValidator>(tcvrValConfig.value());
    XLOG(INFO) << "Enabling transceiver config validation.";
  } else {
    XLOG(INFO) << "Not enabling transceiver config validation.";
  }
}

void TransceiverManager::readWarmBootStateFile() {
  CHECK(canWarmBoot_);

  std::string warmBootJson;
  const auto& warmBootStateFile = warmBootStateFileName();
  if (!folly::readFile(warmBootStateFile.c_str(), warmBootJson)) {
    XLOG(WARN) << "Warm Boot state file: " << warmBootStateFile
               << " doesn't exist, skip restoring warm boot state";
    return;
  }

  warmBootState_ = folly::parseJson(warmBootJson);
}

void TransceiverManager::init() {
  // Check whether we can warm boot
  canWarmBoot_ = checkWarmBootFlags();
  XLOG(INFO) << "Will attempt " << (canWarmBoot_ ? "WARM" : "COLD") << " boot";
  if (canWarmBoot_) {
    // Read the warm boot state file for a warm boot
    readWarmBootStateFile();
    restoreAgentConfigAppliedInfo();
  }

  // Now we might need to start threads
  startThreads();

  // Initialize the PhyManager all ExternalPhy for the system
  initExternalPhyMap();
  // Initialize the I2c bus
  initTransceiverMap();

  // Create data structures for validating transceiver configs
  initTcvrValidator();

  if (!isSystemInitialized_) {
    isSystemInitialized_ = true;
  }
}

QsfpServiceRunState TransceiverManager::getRunState() const {
  if (isExiting()) {
    return QsfpServiceRunState::EXITING;
  }
  if (isUpgradingFirmware()) {
    return QsfpServiceRunState::UPGRADING_FIRMWARE;
  }
  if (isFullyInitialized()) {
    return QsfpServiceRunState::ACTIVE;
  }
  if (isSystemInitialized()) {
    return QsfpServiceRunState::INITIALIZED;
  }
  return QsfpServiceRunState::UNINITIALIZED;
}

void TransceiverManager::restoreAgentConfigAppliedInfo() {
  if (warmBootState_.isNull()) {
    return;
  }
  if (const auto& agentConfigAppliedIt =
          warmBootState_.find(kAgentConfigAppliedInfoStateKey);
      agentConfigAppliedIt != warmBootState_.items().end()) {
    auto agentConfigAppliedInfo = agentConfigAppliedIt->second;
    ConfigAppliedInfo wbConfigAppliedInfo;
    // Restore the last agent config applied timestamp from warm boot state if
    // it exists
    if (const auto& lastAppliedIt =
            agentConfigAppliedInfo.find(kAgentConfigLastAppliedInMsKey);
        lastAppliedIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastAppliedInMs() =
          folly::convertTo<long>(lastAppliedIt->second);
    }
    // Restore the last agent coldboot timestamp from warm boot state if
    // it exists
    if (const auto& lastColdBootIt =
            agentConfigAppliedInfo.find(kAgentConfigLastColdbootAppliedInMsKey);
        lastColdBootIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastColdbootAppliedInMs() =
          folly::convertTo<long>(lastColdBootIt->second);
    }

    configAppliedInfo_ = wbConfigAppliedInfo;
  }
}

void TransceiverManager::clearAllTransceiverReset() {
  {
    auto tscvrsInReset = tcvrsHeldInReset_.rlock();
    if (tscvrsInReset->empty()) {
      qsfpPlatApi_->clearAllTransceiverReset();
    } else {
      const auto numModules = getNumQsfpModules();
      for (auto idx = 0; idx < numModules; idx++) {
        if (tscvrsInReset->count(idx) == 0) {
          // This api accepts 1 based module id however the module id in
          // TransceiverManager is 0 based.
          qsfpPlatApi_->releaseTransceiverReset(idx + 1);
        }
      }
    }
  }
  // Required delay time between a transceiver getting out of reset and fully
  // functional.
  // @lint-ignore CLANGTIDY facebook-hte-BadCall-sleep
  sleep(kSecAfterModuleOutOfReset);
}

void TransceiverManager::hardResetAction(
    void (TransceiverPlatformApi::*func)(unsigned int),
    int idx,
    bool holdInReset,
    bool removeTransceiver) {
  // Order of locking is important.
  auto trscvrsInreset = tcvrsHeldInReset_.wlock();
  if (holdInReset) {
    trscvrsInreset->insert(idx);
  } else {
    trscvrsInreset->erase(idx);
  }
  // This api accepts 1 based module id however the module id in
  // TransceiverManager is 0 based.
  (qsfpPlatApi_.get()->*func)(idx + 1);
  if (removeTransceiver) {
    {
      // Read Lock to trigger all state machine changes
      auto lockedTransceivers = transceivers_.rlock();
      if (auto it = lockedTransceivers->find(TransceiverID(idx));
          it != lockedTransceivers->end()) {
        it->second->removeTransceiver();
        removeTransceiver = true;
      }
    }

    // Write lock to remove the transceiver
    auto lockedTransceivers = transceivers_.wlock();
    lockedTransceivers->erase(TransceiverID(idx));
  }
}

void TransceiverManager::triggerQsfpHardReset(int idx) {
  hardResetAction(
      &TransceiverPlatformApi::triggerQsfpHardReset,
      idx,
      false /*holdInReset*/,
      true /*RemoveTransceiver*/);
  XLOG(INFO) << "triggerQsfpHardReset called for Transceiver: " << idx;
}

void TransceiverManager::holdTransceiverReset(int idx) {
  hardResetAction(
      &TransceiverPlatformApi::holdTransceiverReset,
      idx,
      true /*holdInReset*/,
      true /*RemoveTransceiver*/);
  XLOG(INFO) << "holdTransceiverReset called for Transceiver: " << idx;
}

void TransceiverManager::releaseTransceiverReset(int idx) {
  hardResetAction(
      &TransceiverPlatformApi::releaseTransceiverReset,
      idx,
      false /*holdInReset*/,
      false /*RemoveTransceiver*/);
  XLOG(INFO) << "releaseTransceiverReset called for Transceiver: " << idx;
}

void TransceiverManager::gracefulExit() {
  steady_clock::time_point begin = steady_clock::now();
  XLOG(INFO) << "[Exit] Starting TransceiverManager graceful exit";
  // Stop all the threads before shutdown
  setGracefulExitingFlag();
  stopThreads();
  steady_clock::time_point stopThreadsDone = steady_clock::now();
  XLOG(INFO) << "[Exit] Stopped all state machine threads. Stop time: "
             << duration_cast<duration<float>>(stopThreadsDone - begin).count();

  // Set all warm boot related files before gracefully shut down
  setWarmBootState();
  setCanWarmBoot();

  // Do a graceful shutdown of the phy.
  if (phyManager_) {
    phyManager_->gracefulExit();
  }

  steady_clock::time_point setWBFilesDone = steady_clock::now();
  XLOG(INFO) << "[Exit] Done creating Warm Boot related files. Stop time: "
             << duration_cast<duration<float>>(setWBFilesDone - stopThreadsDone)
                    .count()
             << std::endl
             << "[Exit] Total TransceiverManager graceful Exit time: "
             << duration_cast<duration<float>>(setWBFilesDone - begin).count();
  XLOG(INFO) << "[Exit] QSFP Service Warm boot state: " << std::endl
             << qsfpServiceWarmbootState_;
}

const TransceiverManager::PortNameMap&
TransceiverManager::getPortNameToModuleMap() const {
  if (portNameToModule_.empty()) {
    const auto& platformPorts = platformMapping_->getPlatformPorts();
    for (const auto& it : platformPorts) {
      auto port = it.second;
      auto transceiverId =
          utility::getTransceiverId(port, platformMapping_->getChips());
      if (!transceiverId) {
        continue;
      }

      auto& portName = *(port.mapping()->name());
      portNameToModule_[portName] = transceiverId.value();
    }
  }

  return portNameToModule_;
}

const std::set<std::string> TransceiverManager::getPortNames(
    TransceiverID tcvrId) const {
  std::set<std::string> ports;
  auto it = portGroupMap_.find(tcvrId);
  if (it != portGroupMap_.end() && !it->second.empty()) {
    for (const auto& port : it->second) {
      ports.insert(*port.mapping()->name());
    }
  }
  return ports;
}

const std::string TransceiverManager::getPortName(TransceiverID tcvrId) const {
  auto portNames = getPortNames(tcvrId);
  return portNames.empty() ? "" : *portNames.begin();
}

std::map<std::string, FirmwareUpgradeData>
TransceiverManager::getPortsRequiringOpticsFwUpgrade() const {
  std::map<std::string, FirmwareUpgradeData> ports;
  if (!isFullyInitialized()) {
    throw FbossError("Service is still initializing...");
  }
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& tcvrIt : *lockedTransceivers) {
    auto firmwareUpgradeData = getFirmwareUpgradeData(*tcvrIt.second);
    if (firmwareUpgradeData.has_value()) {
      ports[getPortName(tcvrIt.first)] = firmwareUpgradeData.value();
    }
  }
  return ports;
}

std::map<std::string, FirmwareUpgradeData>
TransceiverManager::triggerAllOpticsFwUpgrade() {
  std::map<std::string, FirmwareUpgradeData> ports;
  if (!isFullyInitialized()) {
    throw FbossError("Service is still initializing...");
  }
  if (isUpgradingFirmware()) {
    throw FbossError("Service is already upgrading firmware...");
  }
  auto portsForFwUpgrade = getPortsRequiringOpticsFwUpgrade();
  auto tcvrsToUpgradeWLock = tcvrsForFwUpgrade.wlock();

  for (const auto& [portName, _] : portsForFwUpgrade) {
    if (portNameToModule_.find(portName) != portNameToModule_.end()) {
      auto tcvrID = TransceiverID(portNameToModule_[portName]);
      FW_LOG(INFO, tcvrID) << "Selected for FW upgrade";
      tcvrsToUpgradeWLock->insert(TransceiverID(portNameToModule_[portName]));
    }
  }
  return portsForFwUpgrade;
}

bool TransceiverManager::firmwareUpgradeRequired(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(ERR) << "[FWUPG] firmwareUpgradeRequired: Transceiver:" << id
              << " is not in transceiver map";
    return false;
  }
  auto& tcvr = *tcvrIt->second;
  bool canUpgrade = false;
  bool iOevbBusy = false;
  bool present = tcvr.isPresent();
  std::string partNumber = tcvr.getPartNumber();
  bool requiresUpgrade = present && getFirmwareUpgradeData(tcvr).has_value();
  if (forceFirmwareUpgradeForTesting_ || requiresUpgrade) {
    // If we are here, it means that this transceiver is present and has the
    // firmware version mismatch and hence requires upgrade
    // We also need to check that at any time one i2c evb should run firmware
    // upgrade. If not, the other transceivers will be inoperational for a long
    // time until the first transceiver is done upgrading
    {
      auto fwEvbWLock = evbsRunningFirmwareUpgrade_.wlock();
      auto moduleEvb = tcvrIt->second->getEvb();
      if (fwEvbWLock->find(moduleEvb) == fwEvbWLock->end()) {
        fwEvbWLock->emplace(moduleEvb, std::vector<TransceiverID>());
      }
      if (fwEvbWLock->at(moduleEvb).size() <
          FLAGS_max_concurrent_evb_fw_upgrade) {
        fwEvbWLock->at(moduleEvb).push_back(id);
        canUpgrade = true;
      } else {
        iOevbBusy = true;
      }
    } // end of fwEvbWLock lock
  }

  FW_LOG(INFO, tcvr.getID())
      << " firmwareUpgradeRequired: " << " present: " << present
      << " partNumber: " << partNumber << " iOevbBusy:" << iOevbBusy
      << " canUpgrade: " << canUpgrade
      << " requiresUpgrade: " << requiresUpgrade;
  return canUpgrade;
}

std::optional<cfg::Firmware> TransceiverManager::getFirmwareFromCfg(
    Transceiver& tcvr) const {
  int tcvrID = tcvr.getID();
  if (!qsfpConfig_) {
    FW_LOG(DBG4, tcvrID) << " qsfpConfig is NULL. No Firmware to return";
    return std::nullopt;
  }

  const auto& qsfpCfg = qsfpConfig_->thrift;
  auto qsfpCfgFw = qsfpCfg.transceiverFirmwareVersions();
  if (!qsfpCfgFw.has_value()) {
    FW_LOG(DBG4, tcvrID)
        << " transceiverFirmwareVersions is NULL. No Firmware to return";
    return std::nullopt;
  }

  auto cachedTcvrInfo = tcvr.getTransceiverInfo();
  auto vendor = cachedTcvrInfo.tcvrState()->vendor();
  if (!vendor.has_value()) {
    FW_LOG(DBG4, tcvrID) << " Vendor not set. No Firmware to return";
    return std::nullopt;
  }

  auto fwVersionInCfgIt =
      qsfpCfgFw->versionsMap()->find(vendor->get_partNumber());
  if (fwVersionInCfgIt == qsfpCfgFw->versionsMap()->end()) {
    FW_LOG(DBG4, tcvrID)
        << " transceiverFirmwareVersions doesn't have a firmware version for part number "
        << vendor->get_partNumber();
    return std::nullopt;
  }

  return fwVersionInCfgIt->second;
}

std::optional<FirmwareUpgradeData> TransceiverManager::getFirmwareUpgradeData(
    Transceiver& tcvr) const {
  // Returns a FirmwareUpgrade if the current firmware revision is different
  // than the one in qsfp config else std::nullopt
  auto cachedTcvrInfo = tcvr.getTransceiverInfo();
  auto moduleStatus = cachedTcvrInfo.tcvrState()->status();
  int tcvrID = tcvr.getID();
  const std::string partNumber = tcvr.getPartNumber();

  if (!moduleStatus.has_value()) {
    FW_LOG(DBG4, tcvrID)
        << " Part Number " << partNumber
        << " moduleStatus not set. Returning nullopt from getFirmwareUpgradeData";
    return std::nullopt;
  }

  auto fwStatus = moduleStatus->fwStatus();
  if (!fwStatus.has_value()) {
    FW_LOG(DBG4, tcvrID)
        << " Part Number " << partNumber
        << " fwStatus not set. Returning nullopt from getFirmwareUpgradeData";
    return std::nullopt;
  }
  FirmwareUpgradeData fwUpgradeData;
  fwUpgradeData.partNumber() = partNumber;

  auto fwFromConfig = getFirmwareFromCfg(tcvr);
  if (!fwFromConfig.has_value()) {
    FW_LOG(DBG4, tcvrID)
        << " Part Number " << partNumber
        << " Fw not available in config. Returning nullopt from getFirmwareUpgradeData";
    return std::nullopt;
  }

  auto& versions = *fwFromConfig->versions();
  for (auto fwIt : versions) {
    const auto& fwType = fwIt.get_fwType();
    if (fwType == cfg::FirmwareType::APPLICATION && fwStatus->version() &&
        fwIt.get_version() != *fwStatus->version()) {
      FW_LOG(INFO, tcvrID)
          << " Part Number " << partNumber
          << " Application Version in cfg=" << fwIt.get_version()
          << " current operational version= " << *fwStatus->version()
          << ". Returning valid getFirmwareUpgradeData for tcvr="
          << tcvr.getID();
      fwUpgradeData.currentFirmwareVersion() = *fwStatus->version();
      fwUpgradeData.desiredFirmwareVersion() = fwIt.get_version();
      return fwUpgradeData;
    }
    if (fwType == cfg::FirmwareType::DSP && fwStatus->dspFwVer() &&
        fwIt.get_version() != *fwStatus->dspFwVer()) {
      FW_LOG(INFO, tcvrID)
          << " Part Number " << partNumber
          << " DSP Version in cfg=" << fwIt.get_version()
          << " current operational version= " << *fwStatus->dspFwVer()
          << ". Returning valid getFirmwareUpgradeData for tcvr="
          << tcvr.getID();
      fwUpgradeData.currentFirmwareVersion() = *fwStatus->dspFwVer();
      fwUpgradeData.desiredFirmwareVersion() = fwIt.get_version();
      return fwUpgradeData;
    }
    FW_LOG(DBG, tcvrID) << " Part Number " << partNumber << " FW Type Cfg "
                        << apache::thrift::util::enumNameSafe(fwType)
                        << " FW Version CFG " << fwIt.get_version()
                        << " FW Version Status "
                        << fwStatus->version().value_or("NOT_SET");
  }

  FW_LOG(INFO, tcvrID)
      << " Part Number " << partNumber
      << " num versions found: " << versions.size()
      << " Version match in getFirmwareUpgradeData. Not Upgrading";

  // Versions match. No need to upgrade firmware
  return std::nullopt;
}

bool TransceiverManager::upgradeFirmware(Transceiver& tcvr) {
  std::optional<cfg::Firmware> fwFromConfig = getFirmwareFromCfg(tcvr);
  const auto tcvrID = tcvr.getID();
  std::string partNumber = tcvr.getPartNumber();
  if (fwFromConfig.has_value()) {
    FW_LOG(INFO, tcvrID)
        << " Upgrading firmware to the one in qsfp config. PartNumber="
        << partNumber;
  } else {
    FW_LOG(ERR, tcvrID) << " No firmware version found to upgrade. partNumber="
                        << partNumber;
    failedOpticsFwUpgradeCount_++;
    return false;
  }

  std::string fwStorageHandleName = tcvr.getFwStorageHandle();
  if (fwStorageHandleName.empty()) {
    FW_LOG(ERR, tcvrID) << " Can't find the fwStorage handle. Part Number="
                        << partNumber
                        << " fwStorageHandle=" << fwStorageHandleName
                        << ". Skipping fw upgrade";
    failedOpticsFwUpgradeCount_++;
    return false;
  }

  std::vector<std::unique_ptr<FbossFirmware>> fwList;

  auto& fwVersions = *(fwFromConfig->versions());
  for (const auto& fw : fwVersions) {
    fwList.emplace_back(
        fwStorage()->getFirmware(fwStorageHandleName, *fw.version()));

    FW_LOG(INFO, tcvrID) << "Adding FW for upgrade. Firmware type="
                         << apache::thrift::util::enumNameSafe(*fw.fwType())
                         << " Part Number=" << partNumber
                         << " fwStorageHandle=" << fwStorageHandleName
                         << " Version=" << *fw.version();
  }

  auto start = std::chrono::steady_clock::now();
  bool upgradeResult = tcvr.upgradeFirmware(fwList);
  auto end = std::chrono::steady_clock::now();
  auto upgradeTime = static_cast<int>(
      std::chrono::duration_cast<std::chrono::seconds>(end - start).count());

  if (upgradeTime > FLAGS_firmware_upgrade_time_limit) {
    exceededTimeLimitFwUpgradeCount_++;
    int prevMaxTime = maxTimeTakenForFwUpgrade_.load();
    while (prevMaxTime < upgradeTime &&
           !maxTimeTakenForFwUpgrade_.compare_exchange_weak(
               prevMaxTime, upgradeTime)) {
      prevMaxTime = maxTimeTakenForFwUpgrade_.load();
    }
  }

  FW_LOG(INFO, tcvr.getID())
      << " Firmware upgrade time was " << upgradeTime
      << " seconds. Expected time was " << FLAGS_firmware_upgrade_time_limit
      << " seconds.";
  return upgradeResult;
}

void TransceiverManager::doTransceiverFirmwareUpgrade(TransceiverID tcvrID) {
  std::vector<folly::Future<bool>> futResponses;
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(tcvrID);
  if (tcvrIt == lockedTransceivers->end() || !tcvrIt->second->isPresent()) {
    XLOG(INFO) << "[FWUPG] Transceiver:" << tcvrID
               << " not found. Can't do firmware upgrade";
    return;
  }
  auto& tcvr = *tcvrIt->second;
  FW_LOG(INFO, tcvr.getID())
      << " Triggering Transceiver Firmware Upgrade for Part Number="
      << tcvr.getPartNumber();

  auto updateStateInFsdb = [&](bool status) {
    if (FLAGS_publish_stats_to_fsdb) {
      auto tcvrInfo = tcvr.updateFwUpgradeStatusInTcvrInfoLocked(status);
      updateTcvrStateInFsdb(tcvrIt->first, std::move(*tcvrInfo.tcvrState()));
    }
  };

  updateStateInFsdb(true);
  if (upgradeFirmware(*tcvrIt->second)) {
    successfulOpticsFwUpgradeCount_++;
  } else {
    failedOpticsFwUpgradeCount_++;
  }
  // We will leave the fwUpgradeStatus as true for now because we still have a
  // lot to do after upgrading the firmware. The optic goes through reset, the
  // state machine goes back to discovered state. IPHY + XPHY ports get
  // programmed again, Optic itself gets programmed again. Therefore, we'll set
  // the fwUpgradeInProgress status to false after we are truly done getting the
  // optic ready after fw upgrade
}

TransceiverManager::TransceiverToStateMachineHelper
TransceiverManager::setupTransceiverToStateMachineHelper() {
  TransceiverToStateMachineHelper stateMachineMap;
  for (auto chip : platformMapping_->getChips()) {
    if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
      continue;
    }
    auto tcvrID = TransceiverID(*chip.second.physicalID());
    stateMachineMap.emplace(
        tcvrID, std::make_unique<TransceiverStateMachineHelper>(this, tcvrID));
  }
  return stateMachineMap;
}

TransceiverManager::TransceiverToPortInfo
TransceiverManager::setupTransceiverToPortInfo() {
  TransceiverToPortInfo tcvrToPortInfo;
  for (auto chip : platformMapping_->getChips()) {
    if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
      continue;
    }
    auto tcvrID = TransceiverID(*chip.second.physicalID());
    auto portToPortInfo = std::make_unique<
        folly::Synchronized<std::unordered_map<PortID, TransceiverPortInfo>>>();
    tcvrToPortInfo.emplace(tcvrID, std::move(portToPortInfo));
  }
  return tcvrToPortInfo;
}

void TransceiverManager::startThreads() {
  // Setup all TransceiverStateMachineHelper thread
  for (auto& stateMachineHelper : stateMachines_) {
    stateMachineHelper.second->startThread();
    heartbeats_.push_back(stateMachineHelper.second->getThreadHeartbeat());
  }

  XLOG(DBG2) << "Started TransceiverStateMachineUpdateThread";
  updateEventBase_ = std::make_unique<folly::EventBase>();
  updateThread_.reset(new std::thread([=, this] {
    this->threadLoop(
        "TransceiverStateMachineUpdateThread", updateEventBase_.get());
  }));

  auto heartbeatStatsFunc = [this](int /* delay */, int /* backLog */) {};
  updateThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      "updateThreadHeartbeat",
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc);
  heartbeats_.push_back(updateThreadHeartbeat_);

  // Create a watchdog that will monitor the heartbeats of all the threads and
  // increment the missed counter when there is no heartbeat on at least one
  // thread in the last FLAGS_state_machine_update_thread_heartbeat_ms * 10 time
  heartbeatWatchdog_ = std::make_unique<ThreadHeartbeatWatchdog>(
      std::chrono::milliseconds(
          FLAGS_state_machine_update_thread_heartbeat_ms * 10),
      [this]() {
        stateMachineThreadHeartbeatMissedCount_ += 1;
        tcData().setCounter(
            kStateMachineThreadHeartbeatMissed,
            stateMachineThreadHeartbeatMissedCount_);
      });
  // Initialize the kStateMachineThreadHeartbeatMissed counter
  tcData().setCounter(kStateMachineThreadHeartbeatMissed, 0);
  // Start monitoring the heartbeats of all the threads
  for (auto heartbeat : heartbeats_) {
    heartbeatWatchdog_->startMonitoringHeartbeat(heartbeat);
  }
  // Kick off the heartbeat monitoring
  heartbeatWatchdog_->start();
}

void TransceiverManager::stopThreads() {
  if (heartbeatWatchdog_) {
    heartbeatWatchdog_->stop();
    heartbeatWatchdog_.reset();
  }
  for (auto heartbeat_ : heartbeats_) {
    heartbeat_.reset();
  }

  // We use runInEventBaseThread() to terminateLoopSoon() rather than calling it
  // directly here.  This ensures that any events already scheduled via
  // runInEventBaseThread() will have a chance to run.
  if (updateThread_) {
    updateEventBase_->runInEventBaseThread(
        [this] { updateEventBase_->terminateLoopSoon(); });
    updateThread_->join();
    XLOG(DBG2) << "Terminated TransceiverStateMachineUpdateThread";
  }
  // Drain any pending updates by calling handlePendingUpdates directly.
  bool updatesDrained = false;
  do {
    handlePendingUpdates();
    {
      std::unique_lock guard(pendingUpdatesLock_);
      updatesDrained = pendingUpdates_.empty();
    }
  } while (!updatesDrained);
  // And finally stop all TransceiverStateMachineHelper thread
  for (auto& stateMachineHelper : stateMachines_) {
    stateMachineHelper.second->stopThread();
  }
}

void TransceiverManager::threadLoop(
    folly::StringPiece name,
    folly::EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

void TransceiverManager::updateStateBlocking(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = updateStateBlockingWithoutWait(id, event);
  if (result) {
    result->wait();
  }
}

std::shared_ptr<BlockingTransceiverStateMachineUpdateResult>
TransceiverManager::enqueueStateUpdateForTcvrWithoutExecuting(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = std::make_shared<BlockingTransceiverStateMachineUpdateResult>();
  auto update = std::make_unique<BlockingTransceiverStateMachineUpdate>(
      id, event, result);
  if (enqueueStateUpdate(std::move(update))) {
    // Only return blocking result if the update has been added in queue
    return result;
  }
  return nullptr;
}

std::shared_ptr<BlockingTransceiverStateMachineUpdateResult>
TransceiverManager::updateStateBlockingWithoutWait(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = std::make_shared<BlockingTransceiverStateMachineUpdateResult>();
  auto update = std::make_unique<BlockingTransceiverStateMachineUpdate>(
      id, event, result);
  if (updateState(std::move(update))) {
    // Only return blocking result if the update has been added in queue
    return result;
  }
  return nullptr;
}

bool TransceiverManager::enqueueStateUpdate(
    std::unique_ptr<TransceiverStateMachineUpdate> update) {
  if (isExiting_) {
    XLOG(WARN) << "Skipped queueing update:" << update->getName()
               << ", since exit already started";
    return false;
  }
  if (!updateEventBase_) {
    XLOG(WARN) << "Skipped queueing update:" << update->getName()
               << ", since updateEventBase_ is not created yet";
    return false;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }
  return true;
}

void TransceiverManager::executeStateUpdates() {
  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_->runInEventBaseThread(handlePendingUpdatesHelper, this);
}

bool TransceiverManager::updateState(
    std::unique_ptr<TransceiverStateMachineUpdate> update) {
  if (!enqueueStateUpdate(std::move(update))) {
    return false;
  }
  // Signal the update thread that updates are pending.
  executeStateUpdates();
  return true;
}

void TransceiverManager::handlePendingUpdatesHelper(TransceiverManager* mgr) {
  return mgr->handlePendingUpdates();
}
void TransceiverManager::handlePendingUpdates() {
  // Get the list of updates to run.
  // We might pull multiple updates off the list at once if several updates were
  // scheduled before we had a chance to process them.
  // In some case we might also end up finding 0 updates to process if a
  // previous handlePendingUpdates() call processed multiple updates.
  StateUpdateList updates;
  std::set<TransceiverID> toBeUpdateTransceivers;
  {
    std::unique_lock guard(pendingUpdatesLock_);
    // Each TransceiverStateMachineUpdate should be able to process at the same
    // time as we already have lock protection in ExternalPhy and QsfpModule.
    // Therefore, we should just put all the pending update into the updates
    // list as long as they are from totally different transceivers.
    auto iter = pendingUpdates_.begin();
    while (iter != pendingUpdates_.end()) {
      auto [_, isInserted] =
          toBeUpdateTransceivers.insert(iter->getTransceiverID());
      if (!isInserted) {
        // Stop when we find another update for the same transceiver
        break;
      }
      ++iter;
    }
    updates.splice(
        updates.begin(), pendingUpdates_, pendingUpdates_.begin(), iter);
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

  XLOG(DBG2) << "About to update " << updates.size()
             << " TransceiverStateMachine";
  // To expedite all these different transceivers state update, use Future
  std::vector<folly::Future<folly::Unit>> stateUpdateTasks;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    TransceiverStateMachineUpdate* update = &(*iter);
    ++iter;

    auto stateMachineItr = stateMachines_.find(update->getTransceiverID());
    if (stateMachineItr == stateMachines_.end()) {
      XLOG(WARN) << "Unrecognize Transceiver:" << update->getTransceiverID()
                 << ", can't find StateMachine for it. Skip updating.";
      delete update;
      continue;
    }

    stateUpdateTasks.push_back(
        folly::via(stateMachineItr->second->getEventBase())
            .thenValue([update, stateMachineItr](auto&&) {
              XLOG(INFO) << "Preparing TransceiverStateMachine update for "
                         << update->getName();
              // Hold the module state machine lock
              const auto& lockedStateMachine =
                  stateMachineItr->second->getStateMachine().wlock();
              update->applyUpdate(*lockedStateMachine);
            })
            .thenError(
                folly::tag_t<std::exception>{},
                [update](const std::exception& ex) {
                  update->onError(ex);
                  delete update;
                }));
  }
  folly::collectAll(stateUpdateTasks).wait();

  // Notify all of the updates of success and delete them.
  while (!updates.empty()) {
    std::unique_ptr<TransceiverStateMachineUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

TransceiverStateMachineState TransceiverManager::getCurrentState(
    TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }

  const auto& lockedStateMachine =
      stateMachineItr->second->getStateMachine().rlock();
  auto curStateOrder = *lockedStateMachine->current_state();
  auto curState = getStateByOrder(curStateOrder);
  XLOG(DBG4) << "Current transceiver:" << static_cast<int32_t>(id)
             << ", state order:" << curStateOrder
             << ", state:" << apache::thrift::util::enumNameSafe(curState);
  return curState;
}

const state_machine<TransceiverStateMachine>&
TransceiverManager::getStateMachineForTesting(TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }
  const auto& lockedStateMachine =
      stateMachineItr->second->getStateMachine().rlock();
  return *lockedStateMachine;
}

bool TransceiverManager::getNeedResetDataPath(TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }
  return stateMachineItr->second->getStateMachine().rlock()->get_attribute(
      needResetDataPath);
}

std::vector<TransceiverID> TransceiverManager::triggerProgrammingEvents() {
  std::vector<TransceiverID> programmedTcvrs;
  int32_t numProgramIphy{0}, numProgramXphy{0}, numProgramTcvr{0},
      numPrepareTcvr{0};
  BlockingStateUpdateResultList results;
  steady_clock::time_point begin = steady_clock::now();
  for (auto& stateMachine : stateMachines_) {
    bool needProgramIphy{false}, needProgramXphy{false}, needProgramTcvr{false},
        moduleStateReady{false};
    {
      const auto& lockedStateMachine =
          stateMachine.second->getStateMachine().rlock();
      needProgramIphy = !lockedStateMachine->get_attribute(isIphyProgrammed);
      needProgramXphy = !lockedStateMachine->get_attribute(isXphyProgrammed);
      needProgramTcvr =
          !lockedStateMachine->get_attribute(isTransceiverProgrammed);
      moduleStateReady =
          (getStateByOrder(*lockedStateMachine->current_state()) ==
           TransceiverStateMachineState::TRANSCEIVER_READY);
    }
    auto tcvrID = stateMachine.first;
    if (needProgramIphy) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_IPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramIphy;
        results.push_back(result);
      }
    } else if (needProgramXphy && phyManager_ != nullptr) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_XPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramXphy;
        results.push_back(result);
      }
    } else if (needProgramTcvr) {
      std::shared_ptr<BlockingTransceiverStateMachineUpdateResult> result{
          nullptr};

      if (moduleStateReady) {
        result = updateStateBlockingWithoutWait(
            tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_TRANSCEIVER);
        if (result) {
          ++numProgramTcvr;
        }
      } else {
        result = updateStateBlockingWithoutWait(
            tcvrID, TransceiverStateMachineEvent::TCVR_EV_PREPARE_TRANSCEIVER);
        if (result) {
          ++numPrepareTcvr;
        }
      }
      if (result) {
        programmedTcvrs.push_back(tcvrID);
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, !programmedTcvrs.empty())
      << "triggerProgrammingEvents has " << numProgramIphy
      << " IPHY programming, " << numProgramXphy << " XPHY programming, "
      << numProgramTcvr << " TCVR programming, " << numPrepareTcvr
      << " TCVR prepare. Total execute time(ms):"
      << duration_cast<milliseconds>(steady_clock::now() - begin).count();
  return programmedTcvrs;
}

void TransceiverManager::programInternalPhyPorts(TransceiverID id) {
  std::map<int32_t, cfg::PortProfileID> programmedIphyPorts;
  if (auto overridePortAndProfileIt =
          overrideTcvrToPortAndProfileForTest_.find(id);
      overridePortAndProfileIt != overrideTcvrToPortAndProfileForTest_.end()) {
    // NOTE: This is only used for testing.
    for (const auto& [portID, profileID] : overridePortAndProfileIt->second) {
      programmedIphyPorts.emplace(portID, profileID);
    }
  } else {
    // Then call wedge_agent programInternalPhyPorts
    auto wedgeAgentClient = utils::createWedgeAgentClient();
    wedgeAgentClient->sync_programInternalPhyPorts(
        programmedIphyPorts, getTransceiverInfo(id), false);
  }

  std::string logStr = folly::to<std::string>(
      "programInternalPhyPorts() for Transceiver=", id, " return [");
  for (const auto& [portID, profileID] : programmedIphyPorts) {
    logStr = folly::to<std::string>(
        logStr,
        portID,
        " : ",
        apache::thrift::util::enumNameSafe(profileID),
        ", ");
  }
  XLOG(INFO) << logStr << "]";

  // Now update the programmed SW port to profile mapping
  if (auto portToPortInfoIt = tcvrToPortInfo_.find(id);
      portToPortInfoIt != tcvrToPortInfo_.end()) {
    auto portToPortInfoWithLock = portToPortInfoIt->second->wlock();
    portToPortInfoWithLock->clear();
    for (auto [portID, profileID] : programmedIphyPorts) {
      TransceiverPortInfo portInfo;
      portInfo.profile = profileID;
      portToPortInfoWithLock->emplace(PortID(portID), portInfo);
    }
  }
}

std::unordered_map<PortID, TransceiverManager::TransceiverPortInfo>
TransceiverManager::getProgrammedIphyPortToPortInfo(TransceiverID id) const {
  if (auto tcvrToPortInfo_It = tcvrToPortInfo_.find(id);
      tcvrToPortInfo_It != tcvrToPortInfo_.end()) {
    return *(tcvrToPortInfo_It->second->rlock());
  }
  return {};
}

void TransceiverManager::resetProgrammedIphyPortToPortInfo(TransceiverID id) {
  if (auto it = tcvrToPortInfo_.find(id); it != tcvrToPortInfo_.end()) {
    auto portToPortInfoWithLock = it->second->wlock();
    portToPortInfoWithLock->clear();
  }
}

std::unordered_map<PortID, cfg::PortProfileID>
TransceiverManager::getOverrideProgrammedIphyPortAndProfileForTest(
    TransceiverID id) const {
  if (auto portAndProfileIt = overrideTcvrToPortAndProfileForTest_.find(id);
      portAndProfileIt != overrideTcvrToPortAndProfileForTest_.end()) {
    return portAndProfileIt->second;
  }
  return {};
}

void TransceiverManager::programExternalPhyPorts(
    TransceiverID id,
    bool needResetDataPath) {
  auto phyManager = getPhyManager();
  if (!phyManager) {
    return;
  }
  // Get programmed iphy port profile
  const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
  if (programmedPortToPortInfo.empty()) {
    // This is due to the iphy ports are disabled. So no need to program xphy
    XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << id
               << ". Can't find programmed iphy port and port info";
    return;
  }
  const auto& supportedXphyPorts = phyManager->getXphyPorts();
  const auto& transceiver = getTransceiverInfo(id);
  for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
    if (std::find(
            supportedXphyPorts.begin(), supportedXphyPorts.end(), portID) ==
        supportedXphyPorts.end()) {
      XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << id
                 << ", Port=" << portID << ". Can't find supported xphy";
      continue;
    }

    phyManager->programOnePort(
        portID, portInfo.profile, transceiver, needResetDataPath);
    XLOG(INFO) << "Programmed XPHY port for Transceiver=" << id
               << ", Port=" << portID << ", Profile="
               << apache::thrift::util::enumNameSafe(portInfo.profile)
               << ", needResetDataPath=" << needResetDataPath;
  }
}

TransceiverInfo TransceiverManager::getTransceiverInfo(TransceiverID id) const {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->getTransceiverInfo();
  } else {
    TransceiverInfo absentTcvr;
    absentTcvr.tcvrState()->present() = false;
    absentTcvr.tcvrState()->port() = id;
    absentTcvr.tcvrState()->timeCollected() = std::time(nullptr);
    absentTcvr.tcvrStats()->timeCollected() = std::time(nullptr);
    return absentTcvr;
  }
}

/*
 * getAllPortSupportedProfiles
 *
 * This function returns the list of all supported port profiles on every port
 * configured by agent config at that moment. If the checkOptics is False then
 * it returns all possible port profiles for every configured port as mentioned
 * in the platform mapping. If the checkOptics is True then it will exclude the
 * port profiles which current optics does not support.
 */
void TransceiverManager::getAllPortSupportedProfiles(
    std::map<std::string, std::vector<cfg::PortProfileID>>&
        supportedPortProfiles,
    bool checkOptics) {
  // Find the list of all available ports from agent config
  std::vector<std::string> availablePorts;

  for (auto& [tcvrId, tcvrPortInfoMap] : tcvrToPortInfo_) {
    auto tcvrPortInfoMapLocked = tcvrPortInfoMap->rlock();
    for (auto& [portId, tcvrPortInfo] : *tcvrPortInfoMapLocked) {
      if (auto portName = getPortNameByPortId(portId)) {
        availablePorts.push_back(portName.value());
      }
    }
  }

  // Get all possible port profiles for all the ports from platform mapping.
  // Exclude the ports which are not configured by agent config
  auto allPossiblePortProfiles = platformMapping_->getAllPortProfiles();
  std::map<std::string, std::vector<cfg::PortProfileID>>
      allConfiguredPortProfiles;
  for (auto& portName : availablePorts) {
    if (allPossiblePortProfiles.find(portName) !=
        allPossiblePortProfiles.end()) {
      allConfiguredPortProfiles[portName] = allPossiblePortProfiles[portName];
    }
  }

  // If we don't need to check the optics support of the profile then return all
  // supported port profiles from the platform mapping which are configured by
  // agent config
  if (!checkOptics) {
    supportedPortProfiles = allConfiguredPortProfiles;
    return;
  }

  for (auto& [portName, portProfiles] : allConfiguredPortProfiles) {
    auto portID = getPortIDByPortName(portName);
    if (!portID.has_value()) {
      continue;
    }
    // Check if the transceiver supports the port profile
    for (auto& profileID : portProfiles) {
      auto tcvrHostLanes = platformMapping_->getTransceiverHostLanes(
          PlatformPortProfileConfigMatcher(
              profileID /* profileID */,
              *portID /* portID */,
              std::nullopt /* portConfigOverrideFactor */));
      if (tcvrHostLanes.empty()) {
        continue;
      }
      auto tcvrStartLane = *tcvrHostLanes.begin();
      auto profileCfgOpt = platformMapping_->getPortProfileConfig(
          PlatformPortProfileConfigMatcher(profileID));
      if (!profileCfgOpt) {
        continue;
      }
      const auto speed = *profileCfgOpt->speed();
      TransceiverPortState portState;
      portState.portName = portName;
      portState.startHostLane = tcvrStartLane;
      portState.speed = speed;
      portState.numHostLanes = tcvrHostLanes.size();
      portState.transmitterTech = profileCfgOpt->iphy()->medium().value_or({});

      auto tcvrIDOpt = getTransceiverID(*portID);
      if (!tcvrIDOpt.has_value()) {
        continue;
      }

      auto lockedTransceivers = transceivers_.rlock();
      auto tcvrIt = lockedTransceivers->find(*tcvrIDOpt);
      if (tcvrIt != lockedTransceivers->end() &&
          tcvrIt->second->tcvrPortStateSupported(portState)) {
        supportedPortProfiles[portName].push_back(profileID);
      }
    }
  }
}

void TransceiverManager::programTransceiver(
    TransceiverID id,
    bool needResetDataPath) {
  // Get programmed iphy port profile
  const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
  if (programmedPortToPortInfo.empty()) {
    // This is due to the iphy ports are disabled. So no need to program tcvr
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Can't find programmed iphy port and port info";
    return;
  }

  ProgramTransceiverState programTcvrState;
  for (const auto& portToPortInfo : programmedPortToPortInfo) {
    auto portProfile = portToPortInfo.second.profile;
    auto portName = getPortNameByPortId(portToPortInfo.first);
    if (!portName.has_value()) {
      throw FbossError(
          "Can't find a portName for portId ", portToPortInfo.first);
    }
    uint8_t tcvrStartLane = 0;
    // Use platform mapping to fetch the transceiver start lane given the port
    // id and profile id
    auto tcvrHostLanes = platformMapping_->getTransceiverHostLanes(
        PlatformPortProfileConfigMatcher(
            portProfile /* profileID */,
            portToPortInfo.first /* portID */,
            std::nullopt /* portConfigOverrideFactor */));
    if (tcvrHostLanes.empty()) {
      throw FbossError("tcvrHostLanes empty for portId ", portToPortInfo.first);
    }
    // tcvrHostLanes is an ordered set. So begin() gives us the first lane
    tcvrStartLane = *tcvrHostLanes.begin();

    if (tcvrStartLane > 8) {
      throw FbossError(
          "Invalid start lane of ",
          tcvrStartLane,
          " for portId ",
          portToPortInfo.first);
    }
    auto profileCfgOpt = platformMapping_->getPortProfileConfig(
        PlatformPortProfileConfigMatcher(portProfile));
    if (!profileCfgOpt) {
      throw FbossError(
          "Can't find profile config for profileID=",
          apache::thrift::util::enumNameSafe(portProfile));
    }
    const auto speed = *profileCfgOpt->speed();
    TransceiverPortState portState;
    portState.portName = *portName;
    portState.startHostLane = tcvrStartLane;
    portState.speed = speed;
    portState.numHostLanes = tcvrHostLanes.size();
    programTcvrState.ports.emplace(*portName, portState);
  }

  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Transeciver is not present";
    return;
  }

  tcvrIt->second->programTransceiver(programTcvrState, needResetDataPath);
  XLOG(INFO) << "Programmed Transceiver for Transceiver=" << id
             << (needResetDataPath ? " with" : " without")
             << " resetting data path";
}

/*
 * readyTransceiver
 *
 * Calls the module type specific function to check their power control
 * configuration and if needed, corrects it. Returns if the module is in ready
 * state to proceed further with programming.
 */
bool TransceiverManager::readyTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip Ready Checking Transceiver=" << id
               << ". Transeciver is not present";
    return true;
  }

  return tcvrIt->second->readyTransceiver();
}

bool TransceiverManager::tryRemediateTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip remediating Transceiver=" << id
               << ". Transeciver is not present";
    return false;
  }
  bool allPortsDown;
  std::vector<std::string> portsToRemediate;
  std::tie(allPortsDown, portsToRemediate) = areAllPortsDown(id);
  bool didRemediate = tcvrIt->second->tryRemediate(
      allPortsDown, pauseRemediationUntil_, portsToRemediate);
  XLOG_IF(INFO, didRemediate)
      << "Remediated Transceiver for Transceiver=" << id
      << " and ports=" << folly::join(",", portsToRemediate);
  return didRemediate;
}

bool TransceiverManager::supportRemediateTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Transceiver=" << id
               << " is not present and can't support remediate";
    return false;
  }
  return tcvrIt->second->supportRemediate();
}

void TransceiverManager::syncNpuPortStatusUpdate(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {
  XLOG(INFO) << "Syncing NPU port status update";
  updateNpuPortStatusCache(portStatus);
  // Update state machine after receiving a new port status update
  updateTransceiverPortStatus();
}

void TransceiverManager::updateNpuPortStatusCache(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {
  npuPortStatusCache_.wlock()->swap(portStatus);
}

void TransceiverManager::updateTransceiverPortStatus() noexcept {
  steady_clock::time_point begin = steady_clock::now();
  std::map<int32_t, NpuPortStatus> newPortToPortStatus;
  std::unordered_set<TransceiverID> tcvrsForFwUpgrade;
  if (!overrideAgentPortStatusForTesting_.empty()) {
    XLOG(WARN) << "[TEST ONLY] Use overrideAgentPortStatusForTesting_ "
               << "for wedge_agent getPortStatus()";
    newPortToPortStatus = overrideAgentPortStatusForTesting_;
  } else if (FLAGS_subscribe_to_state_from_fsdb) {
    newPortToPortStatus = *npuPortStatusCache_.rlock();
  } else {
    try {
      // Then call wedge_agent getPortStatus() to get current port status
      auto wedgeAgentClient = utils::createWedgeAgentClient();
      std::map<int32_t, PortStatus> portStatus;
      wedgeAgentClient->sync_getPortStatus(portStatus, {});
      newPortToPortStatus = getNpuPortStatus(portStatus);
    } catch (const std::exception& ex) {
      // We have retry mechanism to handle failure. No crash here
      XLOG(WARN) << "Failed to call wedge_agent getPortStatus(). "
                 << folly::exceptionStr(ex);
    }
  }
  if (newPortToPortStatus.empty()) {
    XLOG(WARN) << "No port status to process in updateTransceiverPortStatus";
    return;
  }

  int numResetToDiscovered{0}, numResetToNotPresent{0}, numPortStatusChanged{0};
  auto genStateMachineResetEvent =
      [&numResetToDiscovered, &numResetToNotPresent](
          std::optional<TransceiverStateMachineEvent>& event,
          bool isTcvrPresent) {
        // Update present transceiver state machine back to DISCOVERED
        // and absent transeiver state machine back to NOT_PRESENT
        if (event.has_value()) {
          // If event is already set, no-op
          return;
        }
        if (isTcvrPresent) {
          ++numResetToDiscovered;
          event.emplace(
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED);
        } else {
          ++numResetToNotPresent;
          event.emplace(
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_NOT_PRESENT);
        }
      };

  const auto& presentTransceivers = getPresentTransceivers();
  BlockingStateUpdateResultList results;
  for (auto& [tcvrID, portToPortInfo] : tcvrToPortInfo_) {
    std::unordered_set<PortID> statusChangedPorts;
    bool anyPortUp = false;
    bool isTcvrPresent =
        (presentTransceivers.find(tcvrID) != presentTransceivers.end());
    bool isTcvrJustProgrammed =
        (getCurrentState(tcvrID) ==
         TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
    std::optional<TransceiverStateMachineEvent> event;
    { // lock block for portToPortInfo
      auto portToPortInfoWithLock = portToPortInfo->wlock();
      // All possible platform ports for such transceiver
      const auto& portIDs = getAllPlatformPorts(tcvrID);
      for (auto portID : portIDs) {
        auto portStatusIt = newPortToPortStatus.find(portID);
        auto cachedPortInfoIt = portToPortInfoWithLock->find(portID);
        // If portStatus from agent doesn't have such port
        if (portStatusIt == newPortToPortStatus.end()) {
          if (cachedPortInfoIt == portToPortInfoWithLock->end()) {
            continue;
          } else {
            // Agent remove such port, we need to trigger a state machine
            // reset to trigger programming to get the new sw ports
            portToPortInfoWithLock->erase(cachedPortInfoIt);
            genStateMachineResetEvent(event, isTcvrPresent);
          }
        } else { // If portStatus exists
          // But if the port is disabled, we don't need disabled ports in the
          // cache, since we only store enabled ports as we do in the
          // programInternalPhyPorts()
          if (!(portStatusIt->second.portEnabled)) {
            if (cachedPortInfoIt != portToPortInfoWithLock->end()) {
              portToPortInfoWithLock->erase(cachedPortInfoIt);
              genStateMachineResetEvent(event, isTcvrPresent);
            }
          } else {
            // Only care about enabled port status
            anyPortUp = anyPortUp || portStatusIt->second.operState;
            // Agent create such port, we need to trigger a state machine
            // reset to trigger programming to get the new sw ports
            if (cachedPortInfoIt == portToPortInfoWithLock->end()) {
              TransceiverPortInfo portInfo;
              portInfo.status.emplace(portStatusIt->second);
              portToPortInfoWithLock->insert({portID, std::move(portInfo)});
              genStateMachineResetEvent(event, isTcvrPresent);
            } else {
              // Both agent and cache here have such port, update the cached
              // status
              if (!cachedPortInfoIt->second.status ||
                  cachedPortInfoIt->second.status->operState !=
                      portStatusIt->second.operState) {
                statusChangedPorts.insert(portID);
                // If the port just went down, it's a candidate for firmware
                // upgrade
                if (cachedPortInfoIt->second.status &&
                    cachedPortInfoIt->second.status->operState &&
                    !portStatusIt->second.operState) {
                  tcvrsForFwUpgrade.insert(tcvrID);
                }
              }
              cachedPortInfoIt->second.status.emplace(portStatusIt->second);
            }
          }
        }
      }
      // If event is not set, it means not reset event is needed, now check
      // whether we need port status event.
      // Make sure we update active state for a transceiver which just
      // finished programming
      if (!event && ((!statusChangedPorts.empty()) || isTcvrJustProgrammed)) {
        event.emplace(
            anyPortUp ? TransceiverStateMachineEvent::TCVR_EV_PORT_UP
                      : TransceiverStateMachineEvent::TCVR_EV_ALL_PORTS_DOWN);
        ++numPortStatusChanged;
      }

      // Make sure the port event will be added to the update queue under the
      // lock of portToPortInfo, so that it will make sure the cached status
      // and the state machine will be in sync
      if (event.has_value()) {
        if (auto result = updateStateBlockingWithoutWait(tcvrID, *event)) {
          results.push_back(result);
        }
      }
    } // lock block for portToPortInfo
    // After releasing portToPortInfo lock, publishLinkSnapshots() will use
    // transceivers_ lock later
    for (auto portID : statusChangedPorts) {
      try {
        publishLinkSnapshots(portID);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Port " << portID
                  << " failed publishLinkSnapshpts(): " << ex.what();
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(
      DBG2,
      numResetToDiscovered + numResetToNotPresent + numPortStatusChanged > 0)
      << "updateTransceiverPortStatus has " << numResetToDiscovered
      << " transceivers state machines set back to discovered, "
      << numResetToNotPresent << " set back to not_present, "
      << numPortStatusChanged
      << " transceivers need to update port status. Total execute time(ms):"
      << duration_cast<milliseconds>(steady_clock::now() - begin).count();

  if (FLAGS_firmware_upgrade_on_link_down) {
    triggerFirmwareUpgradeEvents(tcvrsForFwUpgrade);
  }
}

void TransceiverManager::triggerFirmwareUpgradeEvents(
    const std::unordered_set<TransceiverID>& tcvrs) {
  if (!FLAGS_firmware_upgrade_supported || tcvrs.empty()) {
    return;
  }
  BlockingStateUpdateResultList results;
  for (auto tcvrID : tcvrs) {
    TransceiverStateMachineEvent event =
        TransceiverStateMachineEvent::TCVR_EV_UPGRADE_FIRMWARE;
    heartbeatWatchdog_->pauseMonitoringHeartbeat(
        stateMachines_.find(tcvrID)->second->getThreadHeartbeat());
    // Only enqueue updates for now, we'll execute them at once after this loop
    if (auto result =
            enqueueStateUpdateForTcvrWithoutExecuting(tcvrID, event)) {
      results.push_back(result);
    }
  }
  if (!results.empty()) {
    isUpgradingFirmware_ = true;
    executeStateUpdates();
    heartbeatWatchdog_->pauseMonitoringHeartbeat(updateThreadHeartbeat_);
    waitForAllBlockingStateUpdateDone(results);

    resetUpgradedTransceiversToDiscovered();
  }
}

void TransceiverManager::updateTransceiverActiveState(
    const std::set<TransceiverID>& tcvrs,
    const std::map<int32_t, PortStatus>& portStatus) noexcept {
  std::map<int32_t, NpuPortStatus> npuPortStatus = getNpuPortStatus(portStatus);
  int numPortStatusChanged{0};
  BlockingStateUpdateResultList results;
  for (auto tcvrID : tcvrs) {
    auto tcvrToPortInfoIt = tcvrToPortInfo_.find(tcvrID);
    if (tcvrToPortInfoIt == tcvrToPortInfo_.end()) {
      XLOG(WARN) << "Unrecoginized Transceiver:" << tcvrID
                 << ", skip updateTransceiverActiveState()";
      continue;
    }
    XLOG(INFO) << "Syncing ports of transceiver " << tcvrID;
    std::unordered_set<PortID> statusChangedPorts;
    bool anyPortUp = false;
    bool isTcvrJustProgrammed =
        (getCurrentState(tcvrID) ==
         TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
    { // lock block for portToPortInfo
      auto portToPortInfoWithLock = tcvrToPortInfoIt->second->wlock();
      for (auto& [portID, tcvrPortInfo] : *portToPortInfoWithLock) {
        // Check whether there's a new port status for such port
        auto portStatusIt = npuPortStatus.find(portID);
        // If port doesn't need to be updated, use the current cached status
        // to indicate whether we need a state update
        if (portStatusIt == npuPortStatus.end()) {
          if (tcvrPortInfo.status) {
            anyPortUp = anyPortUp || tcvrPortInfo.status->operState;
          }
        } else {
          // Only care about enabled port status
          if (portStatusIt->second.portEnabled) {
            anyPortUp = anyPortUp || portStatusIt->second.operState;
            if (!tcvrPortInfo.status ||
                tcvrPortInfo.status->operState !=
                    portStatusIt->second.operState) {
              statusChangedPorts.insert(portID);
              // No need to do the transceiverRefresh() in this code path
              // because that will again enqueue state machine update on i2c
              // event base. That will result in deadlock with
              // stateMachineRefresh() generated update which also runs in
              // same i2c event base
            }
            // And also update the cached port status
            tcvrPortInfo.status = portStatusIt->second;
          }
        }
      }

      // Make sure the port event will be added to the update queue under the
      // lock of portToPortInfo, so that it will make sure the cached status
      // and the state machine will be in sync
      // Make sure we update active state for a transceiver which just
      // finished programming
      if ((!statusChangedPorts.empty()) || isTcvrJustProgrammed) {
        auto event = anyPortUp
            ? TransceiverStateMachineEvent::TCVR_EV_PORT_UP
            : TransceiverStateMachineEvent::TCVR_EV_ALL_PORTS_DOWN;
        ++numPortStatusChanged;
        if (auto result = updateStateBlockingWithoutWait(tcvrID, event)) {
          results.push_back(result);
        }
      }
    } // lock block for portToPortInfo
    // After releasing portToPortInfo lock, publishLinkSnapshots() will use
    // transceivers_ lock later
    for (auto portID : statusChangedPorts) {
      try {
        publishLinkSnapshots(portID);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Port " << portID
                  << " failed publishLinkSnapshpts(): " << ex.what();
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, numPortStatusChanged > 0)
      << "updateTransceiverActiveState has " << numPortStatusChanged
      << " transceivers need to update port status.";
}

void TransceiverManager::resetUpgradedTransceiversToDiscovered() {
  BlockingStateUpdateResultList results;
  std::vector<TransceiverID> tcvrsToReset;
  for (auto& stateMachine : stateMachines_) {
    const auto& lockedStateMachine =
        stateMachine.second->getStateMachine().rlock();
    if (lockedStateMachine->get_attribute(needToResetToDiscovered)) {
      tcvrsToReset.push_back(stateMachine.first);
    }
  }

  if (!tcvrsToReset.empty()) {
    XLOG(INFO)
        << "Resetting the following transceivers to DISCOVERED since they were recently upgraded: "
        << folly::join(",", tcvrsToReset);
    for (auto tcvrID : tcvrsToReset) {
      TransceiverStateMachineEvent event =
          TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED;
      if (auto result = updateStateBlockingWithoutWait(tcvrID, event)) {
        results.push_back(result);
      }
    }
    waitForAllBlockingStateUpdateDone(results);
  }
}

TransceiverValidationInfo TransceiverManager::getTransceiverValidationInfo(
    TransceiverID id,
    bool validatePortProfile) const {
  TransceiverValidationInfo tcvrInfo;
  tcvrInfo.id = id;

  const auto& cachedTcvrInfo = getTransceiverInfo(id);
  const auto& cachedTcvrState = cachedTcvrInfo.tcvrState();
  tcvrInfo.validEepromChecksums = cachedTcvrState->eepromCsumValid().value();

  auto vendor = cachedTcvrState->vendor();
  if (!vendor.has_value() || vendor->name().value().empty()) {
    tcvrInfo.requiredFields = std::make_pair(false, "missingVendor");
    return tcvrInfo;
  }
  tcvrInfo.vendorName = vendor->name().value();

  tcvrInfo.vendorSerialNumber = vendor->serialNumber().value();
  if (vendor->partNumber().value().empty()) {
    tcvrInfo.requiredFields = std::make_pair(false, "missingVendorPartNumber");
    return tcvrInfo;
  }
  tcvrInfo.vendorPartNumber = vendor->partNumber().value();

  auto mediaInterface = cachedTcvrState->moduleMediaInterface();
  tcvrInfo.mediaInterfaceCode = mediaInterface.has_value()
      ? apache::thrift::util::enumNameSafe(mediaInterface.value())
      : "NOVALUE";

  // TODO: Once firmware sync is enabled, consider firmware versions to
  // be required.
  auto moduleStatus = cachedTcvrState->status();
  if (moduleStatus.has_value() && moduleStatus->fwStatus().has_value()) {
    tcvrInfo.firmwareVersion = moduleStatus->fwStatus()->version().value_or("");
    tcvrInfo.dspFirmwareVersion =
        moduleStatus->fwStatus()->dspFwVer().value_or("");
  }

  if (validatePortProfile) {
    const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
    for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
      tcvrInfo.portProfileIds.push_back(portInfo.profile);
    }
    if (!tcvrInfo.portProfileIds.size()) {
      tcvrInfo.requiredFields = std::make_pair(false, "missingPortProfileIds");
    }
  }

  return tcvrInfo;
}

bool TransceiverManager::validateTransceiverById(
    TransceiverID id,
    std::string& notValidatedReason,
    bool validatePortProfile) {
  if (tcvrValidator_ == nullptr) {
    XLOG(DBG5) << "Transceiver Validation not enabled. Skipping.";
    return false;
  }

  TransceiverValidationInfo tcvrInfo =
      getTransceiverValidationInfo(id, validatePortProfile);

  bool isValidated =
      validateTransceiverConfiguration(tcvrInfo, notValidatedReason);
  updateValidationCache(id, isValidated);
  return isValidated;
}

void TransceiverManager::checkPresentThenValidateTransceiver(TransceiverID id) {
  {
    auto lockedTransceivers = transceivers_.rlock();
    if (lockedTransceivers->find(id) == lockedTransceivers->end()) {
      return;
    }
  }
  std::string notValidatedReason;
  validateTransceiverById(id, notValidatedReason, true);
}

std::string TransceiverManager::getTransceiverValidationConfigString(
    TransceiverID id) const {
  if (tcvrValidator_ == nullptr) {
    XLOG(DBG5) << "Transceiver Validation not enabled. Skipping.";
    return "";
  }

  TransceiverValidationInfo tcvrInfo = getTransceiverValidationInfo(id, true);
  std::string notValidatedReason;
  if (validateTransceiverConfiguration(tcvrInfo, notValidatedReason)) {
    return "";
  }

  folly::dynamic r = folly::dynamic::object;
  std::vector<std::string> portProfileIdStrings;
  for (auto portProfileId : tcvrInfo.portProfileIds) {
    portProfileIdStrings.push_back(
        apache::thrift::util::enumNameSafe(portProfileId));
  }

  r["Transceiver ID"] = static_cast<int>(tcvrInfo.id);
  r["Transceiver Vendor"] = tcvrInfo.vendorName;
  r["Transceiver Serial Number"] = tcvrInfo.vendorSerialNumber;
  r["Transceiver Part Number"] = tcvrInfo.vendorPartNumber;
  r["Transceiver Media Interface Code"] = tcvrInfo.mediaInterfaceCode;
  r["Transceiver Application Firmware Version"] = tcvrInfo.firmwareVersion;
  r["Transceiver DSP Firmware Version"] = tcvrInfo.dspFirmwareVersion;
  r["Transceiver Port Profile Ids"] = folly::join(",", portProfileIdStrings);
  r["Non-Validated Attribute"] = notValidatedReason;

  return folly::toPrettyJson(r);
}

int TransceiverManager::getNumNonValidatedTransceiverConfigs(
    const std::map<int32_t, TransceiverInfo>& infoMap) const {
  auto nonValidatedSet = nonValidTransceiversCache_.rlock();
  int numConfigs = 0;

  for (auto& [tcvrId, tcvrInfo] : infoMap) {
    auto isPresent = tcvrInfo.tcvrState()->present().value();
    auto isNonValid =
        nonValidatedSet->find(static_cast<TransceiverID>(tcvrId)) !=
        nonValidatedSet->end();

    if (isPresent && isNonValid) {
      numConfigs++;
    }
  }

  return numConfigs;
}

void TransceiverManager::updateValidationCache(TransceiverID id, bool isValid) {
  auto nonValidatedSet = nonValidTransceiversCache_.wlock();
  if (!isValid) {
    nonValidatedSet->insert(id);
  } else {
    nonValidatedSet->erase(id);
  }
}

void TransceiverManager::refreshStateMachines() {
  XLOG(INFO) << "refreshStateMachines started";
  // Clear the map that tracks the firmware upgrades in progress per evb
  evbsRunningFirmwareUpgrade_.wlock()->clear();

  // Step1: Fetch current port status from wedge_agent.
  // Since the following steps, like refreshTransceivers() might need to use
  // port status to decide whether it's safe to reset a transceiver.
  // Therefore, always do port status update first.
  updateTransceiverPortStatus();

  // Step2: Refresh all transceivers so that we can get an update
  // TransceiverInfo
  const auto& presentXcvrIds = refreshTransceivers();

  bool firstRefreshAfterColdboot = !canWarmBoot_ && !isFullyInitialized();
  std::unordered_set<TransceiverID> potentialTcvrsForFwUpgrade;
  for (auto tcvrID : presentXcvrIds) {
    auto curState = getCurrentState(tcvrID);
    if (curState == TransceiverStateMachineState::INACTIVE &&
        FLAGS_firmware_upgrade_on_link_down) {
      // Anytime a module is in inactive state (link down), it's a candidate for
      // fw upgrade
      XLOG(INFO)
          << "Transceiver " << static_cast<int>(tcvrID)
          << " is in INACTIVE state, adding it to list of potentialTcvrsForFwUpgrade";
      potentialTcvrsForFwUpgrade.insert(tcvrID);
    } else if (
        curState == TransceiverStateMachineState::DISCOVERED &&
        FLAGS_firmware_upgrade_on_coldboot) {
      if (firstRefreshAfterColdboot) {
        // First refresh after cold boot and module is still in
        // discovered state
        XLOG(INFO)
            << "Transceiver " << static_cast<int>(tcvrID)
            << " just did a cold boot and is still in discovered state, adding it to list of potentialTcvrsForFwUpgrade";
        potentialTcvrsForFwUpgrade.insert(tcvrID);
      } else if (FLAGS_firmware_upgrade_on_tcvr_insert) {
        auto stateMachine = stateMachines_.find(tcvrID);
        if (stateMachine != stateMachines_.end() &&
            stateMachine->second->getStateMachine().rlock()->get_attribute(
                newTransceiverInsertedAfterInit)) {
          // Not the first refresh but the module is in discovered state and was
          // just inserted
          XLOG(INFO)
              << "Transceiver " << static_cast<int>(tcvrID)
              << " is in DISCOVERED state and was recently inserted, adding it to list of potentialTcvrsForFwUpgrade";
          potentialTcvrsForFwUpgrade.insert(tcvrID);
        }
      }
    }
  }
  {
    auto tcvrsToUpgradeWLock = tcvrsForFwUpgrade.wlock();
    triggerFirmwareUpgradeEvents(*tcvrsToUpgradeWLock);
    tcvrsToUpgradeWLock->clear();
  }

  if (!potentialTcvrsForFwUpgrade.empty()) {
    triggerFirmwareUpgradeEvents(potentialTcvrsForFwUpgrade);
  }

  // Step3: Check whether there's a wedge_agent config change
  triggerAgentConfigChangeEvent();

  // Step4: Once the transceivers are detected, trigger programming events
  const auto& programmedTcvrs = triggerProgrammingEvents();

  // Step5: Remediate inactive transceivers
  // Only need to remediate ports which are not recently finished
  // programming. Because if they only finished early stage programming like
  // iphy without programming xphy or tcvr, the ports of such transceiver
  // will still be not stable to be remediated.
  std::vector<TransceiverID> stableTcvrs;
  for (auto tcvrID : presentXcvrIds) {
    if (std::find(programmedTcvrs.begin(), programmedTcvrs.end(), tcvrID) ==
        programmedTcvrs.end()) {
      stableTcvrs.push_back(tcvrID);
    }
  }
  triggerRemediateEvents(stableTcvrs);

  // Resume heartbeats at the end of refresh loop in case they were paused by
  // any of the operations above
  for (auto& stateMachine : stateMachines_) {
    heartbeatWatchdog_->resumeMonitoringHeartbeat(
        stateMachine.second->getThreadHeartbeat());
  }
  heartbeatWatchdog_->resumeMonitoringHeartbeat(updateThreadHeartbeat_);

  if (!isFullyInitialized_) {
    isFullyInitialized_ = true;
    // On successful initialization, set warm boot flag in case of a
    // qsfp_service crash (no gracefulExit).
    setCanWarmBoot();
  }
  // Update the warmboot state if there is a change.
  setWarmBootState();

  isUpgradingFirmware_ = false;

  XLOG(INFO) << "refreshStateMachines ended";
}

void TransceiverManager::triggerAgentConfigChangeEvent() {
  auto wedgeAgentClient = utils::createWedgeAgentClient();
  ConfigAppliedInfo newConfigAppliedInfo;
  try {
    wedgeAgentClient->sync_getConfigAppliedInfo(newConfigAppliedInfo);
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "Failed to call wedge_agent getConfigAppliedInfo(). "
               << folly::exceptionStr(ex);

    // For testing only, if overrideAgentConfigAppliedInfoForTesting_ is set,
    // use it directly; otherwise return without trigger any config changed
    // events
    if (overrideAgentConfigAppliedInfoForTesting_) {
      XLOG(INFO)
          << "triggerAgentConfigChangeEvent is using override ConfigAppliedInfo"
          << ", lastAppliedInMs="
          << *overrideAgentConfigAppliedInfoForTesting_->lastAppliedInMs()
          << ", lastColdbootAppliedInMs="
          << (overrideAgentConfigAppliedInfoForTesting_
                      ->lastColdbootAppliedInMs()
                  ? *overrideAgentConfigAppliedInfoForTesting_
                         ->lastColdbootAppliedInMs()
                  : 0);
      newConfigAppliedInfo = *overrideAgentConfigAppliedInfoForTesting_;
    } else {
      return;
    }
  }

  // Now check if the new timestamp is later than the cached one.
  if (*newConfigAppliedInfo.lastAppliedInMs() <=
      *configAppliedInfo_.lastAppliedInMs()) {
    return;
  }

  // Only need to reset data path if there's a new coldboot
  bool resetDataPath = false;
  std::string resetDataPathLog;
  if (auto lastColdbootAppliedInMs =
          newConfigAppliedInfo.lastColdbootAppliedInMs()) {
    if (auto oldLastColdbootAppliedInMs =
            configAppliedInfo_.lastColdbootAppliedInMs()) {
      resetDataPath = (*lastColdbootAppliedInMs > *oldLastColdbootAppliedInMs);
      if (resetDataPath) {
        resetDataPathLog = folly::to<std::string>(
            "Need reset data path. [Old Coldboot time:",
            *oldLastColdbootAppliedInMs,
            ", New Coldboot time:",
            *lastColdbootAppliedInMs,
            "]");
      }
    } else {
      // Always reset data path the cached info doesn't have coldboot config
      // applied time
      resetDataPath = true;
      resetDataPathLog = folly::to<std::string>(
          "Need reset data path. [Old Coldboot time:0, New Coldboot time:",
          *lastColdbootAppliedInMs,
          "]");
    }
  }

  XLOG(INFO) << "New Agent config applied time:"
             << *newConfigAppliedInfo.lastAppliedInMs()
             << " and last cached time:"
             << *configAppliedInfo_.lastAppliedInMs()
             << ". Issue all ports reprogramming events. " << resetDataPathLog;

  // Update present transceiver state machine back to DISCOVERED
  // and absent transeiver state machine back to NOT_PRESENT
  int numResetToDiscovered{0}, numResetToNotPresent{0};
  const auto& presentTransceivers = getPresentTransceivers();
  BlockingStateUpdateResultList results;
  for (auto& stateMachine : stateMachines_) {
    // Only need to set true to `needResetDataPath` attribute here. And leave
    // the state machine to change it to false once it finishes
    // programTransceiver
    if (resetDataPath) {
      stateMachine.second->getStateMachine().wlock()->get_attribute(
          needResetDataPath) = true;
    }
    auto tcvrID = stateMachine.first;
    if (presentTransceivers.find(tcvrID) != presentTransceivers.end()) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID,
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED)) {
        ++numResetToDiscovered;
        results.push_back(result);
      }
    } else {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID,
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_NOT_PRESENT)) {
        ++numResetToNotPresent;
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG(INFO) << "triggerAgentConfigChangeEvent has " << numResetToDiscovered
             << " transceivers state machines set back to discovered, "
             << numResetToNotPresent << " set back to not_present";
  configAppliedInfo_ = newConfigAppliedInfo;
}

TransceiverManager::TransceiverStateMachineHelper::
    TransceiverStateMachineHelper(
        TransceiverManager* tcvrMgrPtr,
        TransceiverID tcvrID)
    : tcvrID_(tcvrID) {
  // Init state should be "TRANSCEIVER_STATE_NOT_PRESENT"
  auto lockedStateMachine = stateMachine_.wlock();
  lockedStateMachine->get_attribute(transceiverMgrPtr) = tcvrMgrPtr;
  lockedStateMachine->get_attribute(transceiverID) = tcvrID_;
  // Make sure this attr is false by default.
  lockedStateMachine->get_attribute(needResetDataPath) = false;
}

void TransceiverManager::TransceiverStateMachineHelper::startThread() {
  updateEventBase_ = std::make_unique<folly::EventBase>("update thread");
  updateThread_.reset(
      new std::thread([this] { updateEventBase_->loopForever(); }));
  auto heartbeatStatsFunc = [this](int /* delay */, int /* backLog */) {};
  heartbeat_ = std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      folly::to<std::string>("stateMachine_", tcvrID_, "_"),
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc);
}

void TransceiverManager::TransceiverStateMachineHelper::stopThread() {
  if (updateThread_) {
    updateEventBase_->terminateLoopSoon();
    updateThread_->join();
  }
}

void TransceiverManager::waitForAllBlockingStateUpdateDone(
    const TransceiverManager::BlockingStateUpdateResultList& results) {
  for (const auto& result : results) {
    if (isExiting_) {
      XLOG(INFO)
          << "Terminating waitForAllBlockingStateUpdateDone for graceful exit";
      return;
    }
    result->wait();
  }
}

/*
 * getPortIDByPortName
 *
 * This function takes the port name string (eth2/1/1) and returns the
 * software port id (or the agent port id) for that
 */
std::optional<PortID> TransceiverManager::getPortIDByPortName(
    const std::string& portName) const {
  auto portMapIt = portNameToPortID_.left.find(portName);
  if (portMapIt != portNameToPortID_.left.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

/*
 * getPortNameByPortId
 *
 * This function takes the software port id and returns corresponding port
 * name string (ie: eth2/1/1)
 */
std::optional<std::string> TransceiverManager::getPortNameByPortId(
    PortID portId) const {
  auto portMapIt = portNameToPortID_.right.find(portId);
  if (portMapIt != portNameToPortID_.right.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

std::vector<PortID> TransceiverManager::getAllPlatformPorts(
    TransceiverID tcvrID) const {
  std::vector<PortID> ports;
  for (const auto& [portID, portInfo] : portToSwPortInfo_) {
    if (portInfo.tcvrID && *portInfo.tcvrID == tcvrID) {
      ports.push_back(portID);
    }
  }
  return ports;
}

std::set<TransceiverID> TransceiverManager::getPresentTransceivers() const {
  std::set<TransceiverID> presentTcvrs;
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& tcvrIt : *lockedTransceivers) {
    if (tcvrIt.second->isPresent()) {
      presentTcvrs.insert(tcvrIt.first);
    }
  }
  return presentTcvrs;
}

void TransceiverManager::setOverrideAgentPortStatusForTesting(
    bool up,
    bool enabled,
    bool clearOnly) {
  // Use overrideTcvrToPortAndProfileForTest_ to prepare
  // overrideAgentPortStatusForTesting_
  overrideAgentPortStatusForTesting_.clear();
  if (clearOnly) {
    return;
  }
  for (const auto& it : overrideTcvrToPortAndProfileForTest_) {
    for (const auto& [portID, profileID] : it.second) {
      NpuPortStatus status;
      status.portEnabled = enabled;
      status.operState = up;
      status.profileID = apache::thrift::util::enumNameSafe(profileID);
      overrideAgentPortStatusForTesting_.emplace(portID, std::move(status));
    }
  }
}

void TransceiverManager::setOverrideAgentConfigAppliedInfoForTesting(
    std::optional<ConfigAppliedInfo> configAppliedInfo) {
  overrideAgentConfigAppliedInfoForTesting_ = configAppliedInfo;
}

std::pair<bool, std::vector<std::string>> TransceiverManager::areAllPortsDown(
    TransceiverID id) const noexcept {
  auto portToPortInfoIt = tcvrToPortInfo_.find(id);
  if (portToPortInfoIt == tcvrToPortInfo_.end()) {
    XLOG(WARN) << "Can't find Transceiver:" << id
               << " in cached tcvrToPortInfo_";
    return {false, {}};
  }
  auto portToPortInfoWithLock = portToPortInfoIt->second->rlock();
  if (portToPortInfoWithLock->empty()) {
    XLOG(WARN) << "Can't find any programmed port for Transceiver:" << id
               << " in cached tcvrToPortInfo_";
    return {false, {}};
  }
  bool anyPortUp = false;
  std::vector<std::string> downPorts;
  for (const auto& [portID, portInfo] : *portToPortInfoWithLock) {
    if (!portInfo.status.has_value()) {
      // If no status set, assume ports are up so we won't trigger any
      // disruptive event
      return {false, {}};
    }
    if (portInfo.status->operState) {
      anyPortUp = true;
    } else {
      auto portName = getPortNameByPortId(portID);
      if (portName.has_value()) {
        downPorts.push_back(*portName);
      }
    }
  }
  return {!anyPortUp, downPorts};
}

bool TransceiverManager::isRunningAsicPrbs(TransceiverID tcvr) const {
  auto ports = getAllPlatformPorts(tcvr);
  for (const auto& port : ports) {
    auto npuPortStatusCacheItr = npuPortStatusCache_.rlock()->find(port);
    if (npuPortStatusCacheItr == npuPortStatusCache_.rlock()->end()) {
      continue;
    }
    if (npuPortStatusCacheItr->second.asicPrbsEnabled) {
      return true;
    }
  }
  return false;
}

void TransceiverManager::triggerRemediateEvents(
    const std::vector<TransceiverID>& stableTcvrs) {
  if (stableTcvrs.empty()) {
    return;
  }
  if (isExiting_) {
    XLOG(INFO) << "Skip triggerRemediateEvents during graceful exit";
    return;
  }
  BlockingStateUpdateResultList results;
  for (auto tcvrID : stableTcvrs) {
    // Check if any of the ports are running ASIC PRBS. If yes, skip triggering
    // remediation on transceiver.
    if (isRunningAsicPrbs(tcvrID)) {
      XLOG(DBG2) << "Skip remediating Transceiver=" << tcvrID
                 << ". Transceiver is running ASIC PRBS";
      continue;
    }

    const auto& programmedPortToPortInfo =
        getProgrammedIphyPortToPortInfo(tcvrID);
    if (programmedPortToPortInfo.empty()) {
      // This is due to the iphy ports are disabled. So no need to remediate
      continue;
    }

    auto curState = getCurrentState(tcvrID);
    // If we are not in the active or inactive state, don't try to remediate
    // yet
    if (curState != TransceiverStateMachineState::ACTIVE &&
        curState != TransceiverStateMachineState::INACTIVE) {
      continue;
    }

    // If we are here because we are in active state, check if any of the
    // ports are down. If yes, try to remediate (partial). If we are here
    // because we are in inactive state, areAllPortsDown will return a
    // non-empty list of down ports anyways, so we will try to remediate
    if (areAllPortsDown(tcvrID).second.empty()) {
      continue;
    }

    // Then check whether we should remediate so that we don't have to create
    // too many unnecessary state machine update
    auto lockedTransceivers = transceivers_.rlock();
    auto tcvrIt = lockedTransceivers->find(tcvrID);
    if (tcvrIt == lockedTransceivers->end()) {
      XLOG(DBG2) << "Skip remediating Transceiver=" << tcvrID
                 << ". Transceiver is not present";
      continue;
    }
    if (!tcvrIt->second->shouldRemediate(pauseRemediationUntil_)) {
      continue;
    }
    if (auto result = updateStateBlockingWithoutWait(
            tcvrID,
            TransceiverStateMachineEvent::TCVR_EV_REMEDIATE_TRANSCEIVER)) {
      results.push_back(result);
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(INFO, !results.empty())
      << "triggerRemediateEvents has " << results.size()
      << " transceivers kicked off remediation";
}

void TransceiverManager::markLastDownTime(TransceiverID id) noexcept {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip markLastDownTime for Transceiver=" << id
               << ". Transeciver is not present";
    return;
  }
  tcvrIt->second->markLastDownTime();
}

time_t TransceiverManager::getLastDownTime(TransceiverID id) const {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    throw FbossError(
        "Can't find Transceiver=", id, ". Transceiver is not present");
  }
  return tcvrIt->second->getLastDownTime();
}

void TransceiverManager::getAllInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos) {
  for (auto [portName, _] : getPortNameToModuleMap()) {
    getInterfacePhyInfo(phyInfos, portName);
  }
}

void TransceiverManager::getInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos,
    const std::string& portName) {
  auto portIDOpt = getPortIDByPortName(portName);
  if (!portIDOpt) {
    throw FbossError(
        "Unrecoginized portName:", portName, ", can't find port id");
  }
  try {
    phyInfos[portName] = getXphyInfo(*portIDOpt);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Fetching PhyInfo for " << portName << " failed with "
              << ex.what();
  }
}

void TransceiverManager::publishLinkSnapshots(std::string portName) {
  auto portIDOpt = getPortIDByPortName(portName);
  if (!portIDOpt) {
    throw FbossError(
        "Unrecoginized portName:", portName, ", can't find port id");
  }
  publishLinkSnapshots(*portIDOpt);
}

void TransceiverManager::publishLinkSnapshots(PortID portID) {
  // Publish xphy snapshots if there's a phyManager and xphy ports
  if (phyManager_) {
    phyManager_->publishXphyInfoSnapshots(portID);
  }
  // Publish transceiver snapshots if there's a transceiver
  if (auto tcvrIDOpt = getTransceiverID(portID)) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrIt = lockedTransceivers->find(*tcvrIDOpt);
        tcvrIt != lockedTransceivers->end()) {
      tcvrIt->second->publishSnapshots();
    }
  }
}

std::optional<TransceiverID> TransceiverManager::getTransceiverID(
    PortID portID) const {
  auto swPortInfo = portToSwPortInfo_.find(portID);
  if (swPortInfo == portToSwPortInfo_.end()) {
    throw FbossError("Failed to find SwPortInfo for port ID ", portID);
  }
  return swPortInfo->second.tcvrID;
}

bool TransceiverManager::verifyEepromChecksums(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip verifying eeprom checksum for Transceiver=" << id
               << ". Transceiver is not present";
    return true;
  }
  return tcvrIt->second->verifyEepromChecksums();
}

bool TransceiverManager::checkWarmBootFlags() {
  // Return true if coldBootOnceFile does not exist and canWarmBoot file
  // exists
  const auto& forceColdBootFile = forceColdBootFileName();
  const auto& warmBootFile = warmBootFlagFileName();

  bool forceColdBoot = removeFile(forceColdBootFile);
  if (forceColdBoot || !FLAGS_can_qsfp_service_warm_boot) {
    XLOG(INFO) << "Force Cold Boot file: " << forceColdBootFile
               << " is set. Removing Warm Boot file " << warmBootFile;
    removeWarmBootFlag();
    return false;
  }

  // Instead of removing the can_warm_boot file, we keep it unless it's a
  // coldboot, so that qsfp_service crash can still use warm boot.
  bool canWarmBoot = checkFileExists(warmBootFile);
  XLOG(INFO) << "Warm Boot flag: " << warmBootFile << " is "
             << (canWarmBoot ? "set" : "missing");
  return canWarmBoot;
}

void TransceiverManager::removeWarmBootFlag() {
  removeFile(warmBootFlagFileName());
}

std::string TransceiverManager::forceColdBootFileName() {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kForceColdBootFileName);
}

std::string TransceiverManager::warmBootFlagFileName() {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kWarmBootFlag);
}

std::string TransceiverManager::warmBootStateFileName() const {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kWarmbootStateFileName);
}

void TransceiverManager::setWarmBootState() {
  // Store necessary information of qsfp_service state into the warmboot state
  // file. This can be the lane id vector of each port from PhyManager or
  // transceiver info or the last config applied timestamp from agent
  folly::dynamic qsfpServiceState = folly::dynamic::object;
  steady_clock::time_point begin = steady_clock::now();
  if (phyManager_) {
    qsfpServiceState[kPhyStateKey] = phyManager_->getWarmbootState();
  }

  folly::dynamic agentConfigAppliedWbState = folly::dynamic::object;
  agentConfigAppliedWbState[kAgentConfigLastAppliedInMsKey] =
      *configAppliedInfo_.lastAppliedInMs();
  if (auto lastAgentColdBootTime =
          configAppliedInfo_.lastColdbootAppliedInMs()) {
    agentConfigAppliedWbState[kAgentConfigLastColdbootAppliedInMsKey] =
        *lastAgentColdBootTime;
  }
  qsfpServiceState[kAgentConfigAppliedInfoStateKey] = agentConfigAppliedWbState;

  std::string currentState = folly::toPrettyJson(qsfpServiceState);
  // If there is a state change, write it to the warm boot state file.
  if (qsfpServiceWarmbootState_ != currentState) {
    // Update the warmboot state
    qsfpServiceWarmbootState_ = currentState;

    steady_clock::time_point getWarmbootState = steady_clock::now();
    XLOG(INFO)
        << "Finish updating warm boot state. Time: "
        << duration_cast<duration<float>>(getWarmbootState - begin).count();
    folly::writeFile(
        qsfpServiceWarmbootState_, warmBootStateFileName().c_str());
    steady_clock::time_point serializeState = steady_clock::now();
    XLOG(INFO) << "Finish writing warm boot state to file. Time: "
               << duration_cast<duration<float>>(
                      serializeState - getWarmbootState)
                      .count();
  }
}

void TransceiverManager::setCanWarmBoot() {
  const auto& warmBootFile = warmBootFlagFileName();
  auto createFd = createFile(warmBootFile);
  close(createFd);
  XLOG(INFO) << "Wrote can warm boot flag: " << warmBootFile;
}

void TransceiverManager::restoreWarmBootPhyState() {
  // Only need to restore warm boot state if this is a warm boot
  if (!canWarmBoot_) {
    XLOG(INFO) << "[Cold Boot] No need to restore warm boot state";
    return;
  }

  if (const auto& phyStateIt = warmBootState_.find(kPhyStateKey);
      phyManager_ && phyStateIt != warmBootState_.items().end()) {
    phyManager_->restoreFromWarmbootState(phyStateIt->second);
  }
}

namespace {
phy::Side prbsComponentToPhySide(phy::PortComponent component) {
  switch (component) {
    case phy::PortComponent::ASIC:
      throw FbossError("qsfp_service doesn't support program ASIC prbs");
    case phy::PortComponent::GB_SYSTEM:
    case phy::PortComponent::TRANSCEIVER_SYSTEM:
      return phy::Side::SYSTEM;
    case phy::PortComponent::GB_LINE:
    case phy::PortComponent::TRANSCEIVER_LINE:
      return phy::Side::LINE;
  };
  throw FbossError(
      "Unsupported prbs component: ",
      apache::thrift::util::enumNameSafe(component));
}
} // namespace

void TransceiverManager::setInterfacePrbs(
    std::string portName,
    phy::PortComponent component,
    const prbs::InterfacePrbsState& state) {
  // Get the port ID first
  auto portId = getPortIDByPortName(portName);
  if (!portId.has_value()) {
    throw FbossError("Can't find a portID for portName ", portName);
  }

  // Sanity check
  if (!state.generatorEnabled().has_value() &&
      !state.checkerEnabled().has_value()) {
    throw FbossError("Neither generator or checker specified for PRBS setting");
  }

  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    if (auto tcvrID = getTransceiverID(portId.value())) {
      phy::Side side = prbsComponentToPhySide(component);
      auto lockedTransceivers = transceivers_.rlock();
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        if (!it->second->setPortPrbs(portName, side, state)) {
          throw FbossError("Failed to set PRBS on transceiver ", *tcvrID);
        }
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId.value());
    }
  } else {
    if (!phyManager_) {
      throw FbossError("Current platform doesn't support xphy");
    }
    // PhyManager is using old portPrbsState
    phy::PortPrbsState phyPrbs;
    phyPrbs.polynominal() = static_cast<int>(state.polynomial().value());
    phyPrbs.enabled() = (state.generatorEnabled().has_value() &&
                         state.generatorEnabled().value()) ||
        (state.checkerEnabled().has_value() && state.checkerEnabled().value());
    phyManager_->setPortPrbs(
        portId.value(), prbsComponentToPhySide(component), phyPrbs);
  }
}

phy::PrbsStats TransceiverManager::getPortPrbsStats(
    PortID portId,
    phy::PortComponent component) const {
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    auto portName = getPortNameByPortId(portId);
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrID = getTransceiverID(portId)) {
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        if (portName.has_value()) {
          return it->second->getPortPrbsStats(portName.value(), side);
        } else {
          throw FbossError("Can't find a portName for portId ", portId);
        }
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId);
    }
  } else {
    if (!phyManager_) {
      throw FbossError("Current platform doesn't support xphy");
    }
    phy::PrbsStats stats;
    auto lanePrbsStats = phyManager_->getPortPrbsStats(portId, side);
    for (const auto& lane : lanePrbsStats) {
      stats.laneStats()->push_back(lane);
      auto timeCollected = lane.timeCollected().value();
      // Store most recent timeCollected across all lane stats
      if (timeCollected > stats.timeCollected()) {
        stats.timeCollected() = timeCollected;
      }
    }
    stats.portId() = portId;
    stats.component() = component;
    return stats;
  }
}

void TransceiverManager::clearPortPrbsStats(
    PortID portId,
    phy::PortComponent component) {
  auto portName = getPortNameByPortId(portId);
  if (!portName.has_value()) {
    throw FbossError("Can't find a portName for portId ", portId);
  }
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrID = getTransceiverID(portId)) {
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        it->second->clearTransceiverPrbsStats(*portName, side);
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId);
    }
  } else if (!phyManager_) {
    throw FbossError("Current platform doesn't support xphy");
  } else {
    phyManager_->clearPortPrbsStats(portId, prbsComponentToPhySide(component));
  }
}

std::vector<prbs::PrbsPolynomial>
TransceiverManager::getTransceiverPrbsCapabilities(
    TransceiverID tcvrID,
    phy::Side side) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(tcvrID);
      it != lockedTransceivers->end()) {
    return it->second->getPrbsCapabilities(side);
  }
  return std::vector<prbs::PrbsPolynomial>();
}

void TransceiverManager::getSupportedPrbsPolynomials(
    std::vector<prbs::PrbsPolynomial>& prbsCapabilities,
    std::string portName,
    phy::PortComponent component) {
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    if (portNameToModule_.find(portName) == portNameToModule_.end()) {
      throw FbossError("Can't find transceiver module for port ", portName);
    }
    prbsCapabilities = getTransceiverPrbsCapabilities(
        TransceiverID(portNameToModule_[portName]), side);
  } else {
    throw FbossError(
        "PRBS on ",
        apache::thrift::util::enumNameSafe(component),
        " not supported by qsfp_service");
  }
}

void TransceiverManager::setPortPrbs(
    PortID portId,
    phy::PortComponent component,
    const phy::PortPrbsState& state) {
  auto portName = getPortNameByPortId(portId);
  if (!portName.has_value()) {
    throw FbossError("Can't find a portName for portId ", portId);
  }

  prbs::InterfacePrbsState newState;
  newState.polynomial() = prbs::PrbsPolynomial(state.polynominal().value());
  newState.generatorEnabled() = state.enabled().value();
  newState.checkerEnabled() = state.enabled().value();
  setInterfacePrbs(portName.value(), component, newState);
}

void TransceiverManager::getInterfacePrbsState(
    prbs::InterfacePrbsState& prbsState,
    const std::string& portName,
    phy::PortComponent component) const {
  if (auto portID = getPortIDByPortName(portName)) {
    if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::TRANSCEIVER_LINE) {
      if (auto tcvrID = getTransceiverID(*portID)) {
        phy::Side side = prbsComponentToPhySide(component);
        auto lockedTransceivers = transceivers_.rlock();
        if (auto it = lockedTransceivers->find(*tcvrID);
            it != lockedTransceivers->end()) {
          prbsState = it->second->getPortPrbsState(portName, side);
          return;
        } else {
          throw FbossError("Can't find transceiver ", *tcvrID);
        }
      } else {
        throw FbossError("Can't find transceiverID for portID ", *portID);
      }
    } else {
      throw FbossError(
          "getInterfacePrbsState not supported on component ",
          apache::thrift::util::enumNameSafe(component));
    }
  } else {
    throw FbossError("Can't find a portID for portName ", portName);
  }
}

void TransceiverManager::getAllInterfacePrbsStates(
    std::map<std::string, prbs::InterfacePrbsState>& prbsStates,
    phy::PortComponent component) const {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  for (const auto& platformPort : platformPorts) {
    auto portName = platformPort.second.mapping()->name_ref();
    try {
      prbs::InterfacePrbsState prbsState;
      getInterfacePrbsState(prbsState, *portName, component);
      prbsStates[*portName] = prbsState;
    } catch (const std::exception& ex) {
      // If PRBS is not enabled on this port, return
      // a default stats / State.
      XLOG(DBG2) << "Failed to get prbs state for port " << *portName
                 << " with error: " << ex.what();
      prbsStates[*portName] = prbs::InterfacePrbsState();
    }
  }
}

phy::PrbsStats TransceiverManager::getInterfacePrbsStats(
    const std::string& portName,
    phy::PortComponent component) const {
  if (auto portID = getPortIDByPortName(portName)) {
    return getPortPrbsStats(*portID, component);
  }
  throw FbossError("Can't find a portID for portName ", portName);
}

void TransceiverManager::getAllInterfacePrbsStats(
    std::map<std::string, phy::PrbsStats>& prbsStats,
    phy::PortComponent component) const {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  for (const auto& platformPort : platformPorts) {
    auto portName = platformPort.second.mapping()->name_ref();
    try {
      auto prbsStatsEntry = getInterfacePrbsStats(*portName, component);
      prbsStats[*portName] = prbsStatsEntry;
    } catch (const std::exception& ex) {
      // If PRBS is not enabled on this port, return
      // a default stats / State.
      XLOG(DBG2) << "Failed to get prbs stats for port " << *portName
                 << " with error: " << ex.what();
      prbsStats[*portName] = phy::PrbsStats();
    }
  }
}

void TransceiverManager::clearInterfacePrbsStats(
    std::string portName,
    phy::PortComponent component) {
  if (auto portID = getPortIDByPortName(portName)) {
    clearPortPrbsStats(*portID, component);
  } else {
    throw FbossError("Can't find a portID for portName ", portName);
  }
}

void TransceiverManager::bulkClearInterfacePrbsStats(
    std::unique_ptr<std::vector<std::string>> interfaces,
    phy::PortComponent component) {
  for (const auto& interface : *interfaces) {
    clearInterfacePrbsStats(interface, component);
  }
}

std::optional<DiagsCapability> TransceiverManager::getDiagsCapability(
    TransceiverID id) const {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->getDiagsCapability();
  }
  XLOG(WARN) << "Return nullopt DiagsCapability for Transceiver=" << id
             << ". Transeciver is not present";
  return std::nullopt;
}

void TransceiverManager::setDiagsCapability(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->setDiagsCapability();
  }
  XLOG(DBG2) << "Skip setting DiagsCapability for Transceiver=" << id
             << ". Transceiver is not present";
}

Transceiver* FOLLY_NULLABLE TransceiverManager::overrideTransceiverForTesting(
    TransceiverID id,
    std::unique_ptr<Transceiver> overrideTcvr) {
  auto lockedTransceivers = transceivers_.wlock();
  // Keep the same logic as updateTransceiverMap()
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    it->second->removeTransceiver();
    lockedTransceivers->erase(it);
  }
  // Only set the override transceiver if it's not null so that we can support
  // removing transceiver in tests
  if (overrideTcvr) {
    lockedTransceivers->emplace(id, std::move(overrideTcvr));
    return lockedTransceivers->at(id).get();
  } else {
    return nullptr;
  }
}

std::vector<TransceiverID> TransceiverManager::refreshTransceivers(
    const std::unordered_set<TransceiverID>& transceivers) {
  std::vector<TransceiverID> transceiverIds;
  std::vector<folly::Future<folly::Unit>> futs;

  {
    auto lockedTransceivers = transceivers_.rlock();
    auto nTransceivers =
        transceivers.empty() ? lockedTransceivers->size() : transceivers.size();
    XLOG(INFO) << "Start refreshing " << nTransceivers << " transceivers...";

    for (const auto& transceiver : *lockedTransceivers) {
      TransceiverID id = TransceiverID(transceiver.second->getID());
      // If we're trying to refresh a subset and this transceiver is not in
      // that subset, skip it.
      if (!transceivers.empty() &&
          transceivers.find(id) == transceivers.end()) {
        continue;
      }
      XLOG(DBG3) << "Fired to refresh TransceiverID=" << id;
      transceiverIds.push_back(id);
      futs.push_back(transceiver.second->futureRefresh());
    }

    folly::collectAll(futs.begin(), futs.end()).wait();
    XLOG(INFO) << "Finished refreshing " << nTransceivers << " transceivers";
  }

  publishTransceiversToFsdb();

  return transceiverIds;
}

void TransceiverManager::resetTransceiver(
    std::unique_ptr<std::vector<std::string>> portNames,
    ResetType resetType,
    ResetAction resetAction) {
  if (!portNames || portNames->empty()) {
    throw FbossError("Invalid portNames argument");
  }

  // Check that the ResetType and ResetAction pair have a valid function
  // call associated with TransceiverPlatformApi.
  auto itr = resetFunctionMap_.find(std::make_pair(resetType, resetAction));
  if (itr == resetFunctionMap_.end()) {
    throw FbossError(
        "Unsupported reset Type and reset action ", resetType, resetAction);
  }

  // Validate all transceivers before any reset action.
  std::vector<int> transceivers;
  for (auto portName : *portNames) {
    auto itr2 = portNameToModule_.find(portName);
    if (itr2 == portNameToModule_.end()) {
      throw FbossError(
          "Can't find transceiver module for port name: ", portName);
    }
    transceivers.push_back(itr2->second);
  }

  // Perform the proper reset action/type on each port.
  for (auto transceiver : transceivers) {
    itr->second(this, transceiver);
  }
}

void TransceiverManager::setPauseRemediation(
    int32_t timeout,
    std::unique_ptr<std::vector<std::string>> portList) {
  if (!portList.get() || portList->empty()) {
    pauseRemediationUntil_ = std::time(nullptr) + timeout;
  } else {
    auto lockedTransceivers = transceivers_.rlock();
    for (auto port : *portList) {
      if (portNameToModule_.find(port) == portNameToModule_.end()) {
        throw FbossError("Can't find transceiver module for port ", port);
      }

      auto it =
          lockedTransceivers->find(TransceiverID(portNameToModule_.at(port)));
      if (it != lockedTransceivers->end()) {
        it->second->setModulePauseRemediation(timeout);
      }
    }
  }
}

void TransceiverManager::getPauseRemediationUntil(
    std::map<std::string, int32_t>& info,
    std::unique_ptr<std::vector<std::string>> portList) {
  if (!portList.get() || portList->empty()) {
    info["all"] = pauseRemediationUntil_;
  } else {
    auto lockedTransceivers = transceivers_.rlock();
    for (auto port : *portList) {
      if (portNameToModule_.find(port) == portNameToModule_.end()) {
        throw FbossError("Can't find transceiver module for port ", port);
      }
      auto it =
          lockedTransceivers->find(TransceiverID(portNameToModule_.at(port)));
      if (it != lockedTransceivers->end()) {
        info[port] = it->second->getModulePauseRemediationUntil();
      }
    }
  }
}

void TransceiverManager::setPortLoopbackState(
    std::string portName,
    phy::PortComponent component,
    bool setLoopback) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(
        folly::sformat("setPortLoopbackState: Invalid port {}", portName));
  }
  if (component != phy::PortComponent::GB_SYSTEM &&
      component != phy::PortComponent::GB_LINE &&
      component != phy::PortComponent::TRANSCEIVER_SYSTEM &&
      component != phy::PortComponent::TRANSCEIVER_LINE) {
    XLOG(INFO)
        << " TransceiverManager::setPortLoopbackState - component not supported "
        << apache::thrift::util::enumNameSafe(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortLoopbackState Port "
             << static_cast<int>(swPort.value());

  if (component == phy::PortComponent::GB_SYSTEM ||
      component == phy::PortComponent::GB_LINE) {
    getPhyManager()->setPortLoopbackState(
        PortID(swPort.value()), component, setLoopback);
  } else {
    // Get the Transceiver ID
    auto tcvrId = getTransceiverID(swPort.value());
    if (!tcvrId.has_value()) {
      throw FbossError(folly::sformat(
          "setInterfaceTxRx: Transceiver not found for port {}", portName));
    }

    // Finally call the transceiver object for port loopback
    auto lockedTransceivers = transceivers_.rlock();
    if (auto it = lockedTransceivers->find(tcvrId.value());
        it != lockedTransceivers->end()) {
      if (component == phy::PortComponent::TRANSCEIVER_LINE) {
        it->second->setTransceiverLoopback(
            portName, phy::Side::LINE, setLoopback);
      } else {
        it->second->setTransceiverLoopback(
            portName, phy::Side::SYSTEM, setLoopback);
      }
    }
  }
}

void TransceiverManager::setPortAdminState(
    std::string portName,
    phy::PortComponent component,
    bool setAdminUp) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(
        folly::sformat("setPortAdminState: Invalid port {}", portName));
  }
  if (component != phy::PortComponent::GB_SYSTEM &&
      component != phy::PortComponent::GB_LINE) {
    XLOG(INFO)
        << " TransceiverManager::setPortAdminState - component not supported "
        << apache::thrift::util::enumNameSafe(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortAdminState Port "
             << static_cast<int>(swPort.value());
  getPhyManager()->setPortAdminState(
      PortID(swPort.value()), component, setAdminUp);
}

/*
 * setInterfaceTxRx
 *
 * Set the interface Tx/Rx output state as per the request. Currently this API
 * supports the Transceiver system and line side control only. The lanes
 * corresponding to the SW ports are disabled/enabled in transceiver. If the
 * channel mask is explicitly specified then only those channels are
 * enabled/disabled
 */
std::vector<phy::TxRxEnableResponse> TransceiverManager::setInterfaceTxRx(
    const std::vector<phy::TxRxEnableRequest>& txRxEnableRequests) {
  std::vector<phy::TxRxEnableResponse> txRxEnableResponses;
  for (const auto& txRxEnableRequest : txRxEnableRequests) {
    auto portName = txRxEnableRequest.portName().value();
    auto component = txRxEnableRequest.component().value();
    auto enable = txRxEnableRequest.enable().value();
    auto channelMask = txRxEnableRequest.laneMask().to_optional();
    auto direction = txRxEnableRequest.direction().value();

    auto swPort = getPortIDByPortName(portName);
    if (!swPort.has_value()) {
      throw FbossError(
          folly::sformat("setInterfaceTxRx: Invalid port {}", portName));
    }
    if (component != phy::PortComponent::TRANSCEIVER_LINE &&
        component != phy::PortComponent::TRANSCEIVER_SYSTEM) {
      throw FbossError(folly::sformat(
          "TransceiverManager::setInterfaceTxRx - component not supported {}",
          apache::thrift::util::enumNameSafe(component)));
    }
    if (direction == phy::Direction::RECEIVE) {
      throw FbossError(folly::sformat(
          "setInterfaceTxRx: Transceiver Rx lane control not implemented for {}",
          portName));
    }

    XLOG(INFO) << folly::sformat(
        "TransceiverManager::setInterfaceTxRx Port {:s}", portName);

    // Get Transceiver ID for this SW Port
    auto tcvrId = getTransceiverID(swPort.value());
    if (!tcvrId.has_value()) {
      throw FbossError(folly::sformat(
          "setInterfaceTxRx: Transceiver not found for port {}", portName));
    }

    // Finally call the transceiver object with SW Port channel list and
    // optionally user requested channel mask
    auto lockedTransceivers = transceivers_.rlock();
    if (auto it = lockedTransceivers->find(tcvrId.value());
        it != lockedTransceivers->end()) {
      std::optional<uint8_t> tcvrChannelMask{std::nullopt};
      if (channelMask.has_value()) {
        tcvrChannelMask = channelMask.value();
      }

      phy::TxRxEnableResponse response;
      response.portName() = portName;
      auto side = (component == phy::PortComponent::TRANSCEIVER_LINE)
          ? phy::Side::LINE
          : phy::Side::SYSTEM;
      auto result =
          it->second->setTransceiverTx(portName, side, tcvrChannelMask, enable);
      response.success() = result;

      txRxEnableResponses.push_back(response);
    }
  }
  return txRxEnableResponses;
}

/*
 * getSymbolErrorHistogram
 *
 * This function returns the Symbol error histogram for a givem port. The
 * return value is a map of datapath id to CDB symbol error histogram values
 */
void TransceiverManager::getSymbolErrorHistogram(
    CdbDatapathSymErrHistogram& symErr,
    const std::string& portName) {
  std::map<std::string, CdbDatapathSymErrHistogram> symbolErrors;

  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(
        folly::sformat("getSymbolErrorHistogram: Invalid port {}", portName));
  }

  // Get Transceiver ID for this SW Port
  auto tcvrId = getTransceiverID(swPort.value());
  if (!tcvrId.has_value()) {
    throw FbossError(folly::sformat(
        "getSymbolErrorHistogram: Transceiver not found for port {}",
        portName));
  }

  // Finally call the transceiver object with for symbol error get function
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(tcvrId.value());
      it != lockedTransceivers->end()) {
    symbolErrors = it->second->getSymbolErrorHistogram();
  }

  for (auto& [pName, datapathSymErr] : symbolErrors) {
    if (pName != portName) {
      continue;
    }
    for (auto& [bin, datapathBinSymErr] : datapathSymErr.media().value()) {
      symErr.media()[bin] = datapathBinSymErr;
    }
    for (auto& [bin, datapathBinSymErr] : datapathSymErr.host().value()) {
      symErr.host()[bin] = datapathBinSymErr;
    }
  }
}

/*
 * getAllPortPhyInfo
 *
 * Get the map of software port id to PortPhyInfo in the system. This function
 * mainly for debugging
 */
std::map<uint32_t, phy::PhyIDInfo> TransceiverManager::getAllPortPhyInfo() {
  std::map<uint32_t, phy::PhyIDInfo> resultMap;

  auto allPlatformPortsIt = platformMapping_->getPlatformPorts();
  for (auto platformPortIt : allPlatformPortsIt) {
    auto portId = platformPortIt.first;
    GlobalXphyID xphyId;
    try {
      xphyId = phyManager_->getGlobalXphyIDbyPortID(PortID(portId));
    } catch (FbossError&) {
      continue;
    }
    phy::PhyIDInfo phyIdInfo = phyManager_->getPhyIDInfo(xphyId);
    resultMap[portId] = phyIdInfo;
  }

  return resultMap;
}

/*
 * getPhyInfo
 *
 * Returns the phy line params for a port
 */
phy::PhyInfo TransceiverManager::getPhyInfo(const std::string& portName) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(folly::sformat("getPhyInfo: Invalid port {}", portName));
  }
  return getPhyManager()->getPhyInfo(PortID(swPort.value()));
}

std::string TransceiverManager::getPortInfo(std::string portName) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(folly::sformat("getPortInfo: Invalid port {}", portName));
  }
  return getPhyManager()->getPortInfoStr(PortID(swPort.value()));
}

bool TransceiverManager::validateTransceiverConfiguration(
    TransceiverValidationInfo& tcvrInfo,
    std::string& notValidatedReason) const {
  if (tcvrValidator_ == nullptr) {
    return false;
  }
  return tcvrValidator_->validateTcvr(tcvrInfo, notValidatedReason);
}

} // namespace facebook::fboss
