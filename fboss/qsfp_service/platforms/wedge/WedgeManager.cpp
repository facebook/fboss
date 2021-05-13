// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/QsfpConfig.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"

#include <fb303/ThreadCachedServiceData.h>

#include <folly/gen/Base.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

// allow us to configure the warmboot dir so that the qsfp cold boot test can
// run concurrently with itself
DEFINE_string(
    warmboot_dir,
    "/dev/shm/fboss/warm_boot",
    "Path to the directory in which we store the warmboot flag");

namespace {

constexpr int kSecAfterModuleOutOfReset = 2;
constexpr auto kForceColdBootFileName = "cold_boot_once_qsfp_service";

} // namespace

namespace facebook {
namespace fboss {

WedgeManager::WedgeManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping,
    PlatformMode mode)
    : TransceiverManager(std::move(api), std::move(platformMapping)),
      platformMode_(mode) {
  /* Constructor for WedgeManager class:
   * Get the TransceiverPlatformApi object from the creator of this object,
   * this object will be used for controlling the QSFP devices on board.
   * Going foward the qsfpPlatApi_ will be used to controll the QSFP devices
   * on FPGA managed platforms and the wedgeI2cBus_ will be used to control
   * the QSFP devices on I2C/CPLD managed platforms
   */
}

void WedgeManager::loadConfig() {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  agentConfig_ = AgentConfig::fromDefaultFile();

  // Process agent config info here.
  for (const auto& port : *agentConfig_->thrift.sw_ref()->ports_ref()) {
    // Get the transceiver id based on the port info from config.
    auto portId = *port.logicalID_ref();

    auto itPlatformPort = platformPorts.find(portId);
    if (itPlatformPort == platformPorts.end()) {
      XLOG(ERR) << "Did not find platform port for sw port " << portId;
      continue;
    }

    auto transceiverId = utility::getTransceiverId(
        itPlatformPort->second, platformMapping_->getChips());

    if (!transceiverId) {
      XLOG(ERR) << "Did not find transceiver id for port id " << portId;
      continue;
    }

    // Add the port to the transceiver indexed port group.
    auto portGroupIt = portGroupMap_.find(transceiverId.value());
    if (portGroupIt == portGroupMap_.end()) {
      portGroupMap_[transceiverId.value()] = std::set<cfg::Port>{port};
    } else {
      portGroupIt->second.insert(port);
    }
    std::string portName = "";
    if (auto name = port.name_ref()) {
      portName = *name;
      portNameToModule_[portName] = transceiverId.value();
    }
    XLOG(INFO) << "Added port " << portName << " with portId " << portId
               << " to transceiver " << transceiverId.value();
  }

  // Process QSFP config here
  try {
    qsfpConfig_ = QsfpConfig::fromDefaultFile();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error loading QSFP Service config : " << ex.what();
  }
}

void WedgeManager::initTransceiverMap() {
  // If we can't get access to the USB devices, don't bother to
  // create the QSFP objects;  this is likely to be a permanent
  // error.
  try {
    wedgeI2cBus_ = getI2CBus();
  } catch (const I2cError& ex) {
    XLOG(ERR) << "failed to initialize I2C interface: " << ex.what();
    return;
  }

  // Initialize port status map for transceivers.
  for (int idx = 0; idx < getNumQsfpModules(); idx++) {
    ports_.wlock()->emplace(
        TransceiverID(idx), std::map<uint32_t, PortStatus>());
  }

  // Check if a cold boot has been forced
  if (removeFile(folly::to<std::string>(
          FLAGS_warmboot_dir, "/", kForceColdBootFileName))) {
    XLOG(INFO) << "Forced cold boot";
    for (int idx = 0; idx < getNumQsfpModules(); idx++) {
      try {
        // Force hard resets on the transceivers which forces a cold boot of the
        // modules.
        triggerQsfpHardReset(idx);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "failed to triggerQsfpHardReset at idx " << idx << ": "
                  << ex.what();
      }
    }
  } else {
    XLOG(INFO) << "Attempting a warm boot";
  }

  // Also try to load the config file here so that we have transceiver to port
  // mapping and port name recognization.
  loadConfig();

  refreshTransceivers();
}

void WedgeManager::getTransceiversInfo(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiverInfo, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }

  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    TransceiverInfo trans;
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      try {
        trans = it->second->getTransceiverInfo();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << i
                  << ": Error calling getTransceiverInfo(): " << ex.what();
      }
    } else {
      trans.present_ref() = false;
    }
    info[i] = trans;
  }
}

void WedgeManager::getTransceiversRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiversRawDOMData, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    RawDOMData data;
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      try {
        data = it->second->getRawDOMData();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << i
                  << ": Error calling getRawDOMData(): " << ex.what();
      }
      info[i] = data;
    }
  }
}

void WedgeManager::getTransceiversDOMDataUnion(
    std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiversDOMDataUnion, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    DOMDataUnion data;
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      try {
        data = it->second->getDOMDataUnion();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << i
                  << ": Error calling getDOMDataUnion(): " << ex.what();
      }
      info[i] = data;
    }
  }
}

void WedgeManager::readTransceiverRegister(
    std::map<int32_t, ReadResponse>& responses,
    std::unique_ptr<ReadRequest> request) {
  auto ids = *(request->ids_ref());
  XLOG(INFO) << "Received request for reading transceiver registers for ids: "
             << (ids.size() > 0 ? folly::join(",", ids) : "None");
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : ids) {
    ReadResponse resp;
    // Mark the response as invalid initially. It will be marked valid when the
    // read goes through successfully.
    resp.valid_ref() = false;
    if (isValidTransceiver(i)) {
      if (auto it = lockedTransceivers->find(TransceiverID(i));
          it != lockedTransceivers->end()) {
        try {
          auto param = *(request->parameter_ref());
          resp.data_ref() = *(it->second->readTransceiver(param));
          resp.valid_ref() = resp.data_ref()->length() > 0;
        } catch (const std::exception& ex) {
          XLOG(ERR) << "Transceiver " << i
                    << ": Error reading from transceiver " << ex.what();
        }
      }
    }
    responses[i] = resp;
  }
}

void WedgeManager::writeTransceiverRegister(
    std::map<int32_t, WriteResponse>& responses,
    std::unique_ptr<WriteRequest> request) {
  auto ids = *(request->ids_ref());
  XLOG(INFO) << "Received request for writing transceiver register for ids: "
             << (ids.size() > 0 ? folly::join(",", ids) : "None");
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : ids) {
    WriteResponse resp;
    resp.success_ref() = false;
    if (isValidTransceiver(i)) {
      if (auto it = lockedTransceivers->find(TransceiverID(i));
          it != lockedTransceivers->end()) {
        try {
          auto param = *(request->parameter_ref());
          resp.success_ref() =
              it->second->writeTransceiver(param, *(request->data_ref()));
        } catch (const std::exception& ex) {
          XLOG(ERR) << "Transceiver " << i << ": Error writing to transceiver "
                    << ex.what();
        }
      }
    }
    responses[i] = resp;
  }
}

void WedgeManager::customizeTransceiver(int32_t idx, cfg::PortSpeed speed) {
  if (!isValidTransceiver(idx)) {
    return;
  }
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(TransceiverID(idx));
      it != lockedTransceivers->end()) {
    try {
      it->second->customizeTransceiver(speed);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Transceiver " << idx
                << ": Error calling customizeTransceiver(): " << ex.what();
    }
  }
}

void WedgeManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  auto groups = folly::gen::from(*ports) |
      folly::gen::filter([](const std::pair<int32_t, PortStatus>& item) {
                  return item.second.transceiverIdx_ref();
                }) |
      folly::gen::groupBy([](const std::pair<int32_t, PortStatus>& item) {
                  return *item.second.transceiverIdx_ref()
                              .value_unchecked()
                              .transceiverId_ref();
                }) |
      folly::gen::as<std::vector>();

  auto lockedTransceivers = transceivers_.rlock();
  auto lockedPorts = ports_.wlock();
  for (auto& group : groups) {
    int32_t transceiverIdx = group.key();
    auto tcvrID = TransceiverID(transceiverIdx);
    XLOG(INFO) << "Syncing ports of transceiver " << transceiverIdx;
    if (!isValidTransceiver(transceiverIdx)) {
      continue;
    }

    // Update the PortStatus map in WedgeManager.
    for (auto portStatus : group.values()) {
      lockedPorts->at(tcvrID)[portStatus.first] = portStatus.second;
    }

    if (auto it = lockedTransceivers->find(tcvrID);
        it != lockedTransceivers->end()) {
      try {
        auto transceiver = it->second.get();
        transceiver->transceiverPortsChanged(lockedPorts->at(tcvrID));
        info[transceiverIdx] = transceiver->getTransceiverInfo();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << transceiverIdx
                  << ": Error calling syncPorts(): " << ex.what();
      }
    } else {
      XLOG(ERR) << "Syncing ports to a transceiver that is not present.";
    }
  }
}

void WedgeManager::refreshTransceivers() {
  try {
    wedgeI2cBus_->verifyBus(false);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error calling verifyBus(): " << ex.what();
    return;
  }

  clearAllTransceiverReset();

  // Since transceivers may appear or disappear, we need to update our
  // transceiver mapping and type here.
  updateTransceiverMap();

  std::vector<folly::Future<folly::Unit>> futs;
  XLOG(INFO) << "Start refreshing all transceivers...";

  auto lockedTransceivers = transceivers_.rlock();

  for (const auto& transceiver : *lockedTransceivers) {
    XLOG(DBG3) << "Fired to refresh transceiver "
               << transceiver.second->getID();
    futs.push_back(transceiver.second->futureRefresh());
  }

  folly::collectAll(futs.begin(), futs.end()).wait();
  XLOG(INFO) << "Finished refreshing all transceivers";
}

int WedgeManager::scanTransceiverPresence(
    std::unique_ptr<std::vector<int32_t>> ids) {
  // If the id list is empty, we default to scan the presence of all the
  // transcievers.
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }

  std::map<int32_t, ModulePresence> presenceUpdate;
  for (auto id : *ids) {
    presenceUpdate[id] = ModulePresence::UNKNOWN;
  }

  wedgeI2cBus_->scanPresence(presenceUpdate);

  int numTransceiversUp = 0;
  for (const auto& presence : presenceUpdate) {
    if (presence.second == ModulePresence::PRESENT) {
      numTransceiversUp++;
    }
  }
  return numTransceiversUp;
}

void WedgeManager::clearAllTransceiverReset() {
  qsfpPlatApi_->clearAllTransceiverReset();
  // Required delay time between a transceiver getting out of reset and fully
  // functional.
  sleep(kSecAfterModuleOutOfReset);
}

void WedgeManager::triggerQsfpHardReset(int idx) {
  // This api accepts 1 based module id however the module id in
  // WedgeManager is 0 based.
  qsfpPlatApi_->triggerQsfpHardReset(idx + 1);
}

std::unique_ptr<TransceiverI2CApi> WedgeManager::getI2CBus() {
  return std::make_unique<WedgeI2CBusLock>(std::make_unique<WedgeI2CBus>());
}

void WedgeManager::updateTransceiverMap() {
  std::vector<folly::Future<TransceiverManagementInterface>> futInterfaces;
  std::vector<std::unique_ptr<WedgeQsfp>> qsfpImpls;
  for (int idx = 0; idx < getNumQsfpModules(); idx++) {
    qsfpImpls.push_back(std::make_unique<WedgeQsfp>(idx, wedgeI2cBus_.get()));
    futInterfaces.push_back(
        qsfpImpls[idx]->futureGetTransceiverManagementInterface());
  }
  folly::collectAllUnsafe(futInterfaces.begin(), futInterfaces.end()).wait();
  // After we have collected all transceivers, get the write lock on
  // transceivers_ before updating it
  auto lockedTransceivers = transceivers_.wlock();
  auto lockedPorts = ports_.rlock();
  for (int idx = 0; idx < getNumQsfpModules(); idx++) {
    if (!futInterfaces[idx].isReady()) {
      XLOG(ERR) << "failed getting TransceiverManagementInterface at " << idx;
      continue;
    }
    auto it = lockedTransceivers->find(TransceiverID(idx));
    if (it != lockedTransceivers->end()) {
      // In the case where we already have a transceiver recorded, try to check
      // whether they match the transceiver type.
      if (it->second->managementInterface() == futInterfaces[idx].value()) {
        // The management interface matches. Nothing needs to be done.
        continue;
      } else {
        // The management changes. Need to Delete the old module to make place
        // for the new one.
        lockedTransceivers->erase(it);
      }
    }

    // Either we don't have a transceiver here before or we had a new one since
    // the management interface changed, we want to create a new module here.
    int portsPerTransceiver =
        (portGroupMap_.size() == 0 ? numPortsPerTransceiver()
                                   : portGroupMap_[idx].size());
    if (futInterfaces[idx].value() == TransceiverManagementInterface::CMIS) {
      XLOG(INFO) << "making CMIS QSFP for " << idx;
      lockedTransceivers->emplace(
          TransceiverID(idx),
          std::make_unique<CmisModule>(
              this, std::move(qsfpImpls[idx]), portsPerTransceiver));
    } else if (
        futInterfaces[idx].value() == TransceiverManagementInterface::SFF) {
      XLOG(INFO) << "making Sff QSFP for " << idx;
      lockedTransceivers->emplace(
          TransceiverID(idx),
          std::make_unique<SffModule>(
              this, std::move(qsfpImpls[idx]), portsPerTransceiver));
    } else {
      XLOG(ERR) << "Unknown Transceiver interface: "
                << static_cast<int>(futInterfaces[idx].value()) << " at idx "
                << idx;

      try {
        if (!qsfpImpls[idx]->detectTransceiver()) {
          XLOG(DBG3) << "Transceiver is not present at idx " << idx;
          continue;
        }
      } catch (const std::exception& ex) {
        XLOG(ERR) << "failed to detect transceiver at idx " << idx << ": "
                  << ex.what();
        continue;
      }
      // There are times when a module cannot be read however it's present.
      // Try to reset here since that may be able to bring it back.
      bool safeToReset = false;
      if (auto iter = lockedPorts->find(TransceiverID(idx));
          iter != lockedPorts->end()) {
        // Check if we have expected ports info synced over and if all of
        // the port is down. If any of them is not true then we will not
        // perform the reset.
        safeToReset =
            (iter->second.size() == portsPerTransceiver) &&
            std::all_of(
                iter->second.begin(), iter->second.end(), [](const auto& port) {
                  return !(*port.second.up_ref());
                });
      }
      if (safeToReset && (std::time(nullptr) > pauseRemediationUntil_)) {
        XLOG(INFO) << "A present transceiver with unknown interface at " << idx
                   << " Try reset.";
        try {
          triggerQsfpHardReset(idx);
        } catch (const std::exception& ex) {
          XLOG(ERR) << "failed to triggerQsfpHardReset at idx " << idx << ": "
                    << ex.what();
          continue;
        }
      } else {
        XLOG(ERR) << "Unknown interface of transceiver with ports up at "
                  << idx;
      }
      continue;
    }

    // Feed its port status to the newly constructed transceiver.
    // However skip if ports have not been synced initially.
    // transceiverPortsChanged will call refreshLocked which takes close to a
    // second for a transceiver. Calling it for every transceiver at
    // initialization is time consuming. Leaving that for refreshTransceivers
    // which runs concurrently for each transceiver.
    if (auto iter = lockedPorts->find(TransceiverID(idx));
        iter != lockedPorts->end() && !iter->second.empty()) {
      try {
        lockedTransceivers->at(TransceiverID(idx))
            ->transceiverPortsChanged(iter->second);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << idx
                  << ": Error calling transceiverPortsChanged: " << ex.what();
      }
    }
  }
}

/* Get the i2c transaction counters from TranscieverManager base class
 * and update to fbagent. The TransceieverManager base class is inherited
 * by platform speficic Transaceiver Manager class like WedgeManager.
 * That class has the function to get the I2c transaction status.
 */
void WedgeManager::publishI2cTransactionStats() {
  // Get the i2c transaction stats from TransactionManager class (its
  // sub-class having platform specific implementation)
  auto counters = getI2cControllerStats();

  if (counters.size() == 0)
    return;

  // Populate the i2c stats per pim and per controller

  for (const I2cControllerStats& counter : counters) {
    // Publish all the counters to FbAgent

    auto statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".readTotal");
    tcData().setCounter(statName, *counter.readTotal__ref());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".readFailed");
    tcData().setCounter(statName, *counter.readFailed__ref());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".readBytes");
    tcData().setCounter(statName, *counter.readBytes__ref());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".writeTotal");
    tcData().setCounter(statName, *counter.writeTotal__ref());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".writeFailed");
    tcData().setCounter(statName, *counter.writeFailed__ref());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName__ref(), ".writeBytes");
    tcData().setCounter(statName, *counter.writeBytes__ref());
  }
}

/*
 * This is introduced mainly due to the mismatch of ODS reporting frequency
 * and the interval of us reading transceiver data. Some of the clear on read
 * information may be lost in this process and not being captured in the ODS
 * time series. This would bring difficulty in root cause link issues. Thus
 * here we provide a way of read and clear the data for the purpose of ODS
 * data reporting.
 */
void WedgeManager::getAndClearTransceiversSignalFlags(
    std::map<int32_t, SignalFlags>& signalFlagsMap,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "getAndClearTransceiversSignalFlags, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }

  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    SignalFlags signalFlags;
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      signalFlags = it->second->readAndClearCachedSignalFlags();
      signalFlagsMap[i] = signalFlags;
    }
  }
}

void WedgeManager::getAndClearTransceiversMediaSignals(
    std::map<int32_t, std::map<int, MediaLaneSignals>>& mediaSignalsMap,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "getAndClearTransceiversMediaSignals, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }

  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    std::map<int, MediaLaneSignals> mediaSignals;
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      mediaSignals = it->second->readAndClearCachedMediaLaneSignals();
      mediaSignalsMap[i] = mediaSignals;
    }
  }
}

/*
 * getPhyPortConfigValues
 *
 * This function takes the portId and port profile id. Based on these it looks
 * into the platform mapping for the given platform and extracts information
 * to fill in the phy port config. The output of this function is phy port
 * config structure which can be used later to send to External Phy functions
 */
std::optional<phy::PhyPortConfig> WedgeManager::getPhyPortConfigValues(
    int32_t portId,
    cfg::PortProfileID portProfileId) {
  phy::PhyPortConfig phyPortConfig;

  // First verify if the platform mapping exist for this platform
  if (platformMapping_.get() == nullptr) {
    XLOG(INFO) << "Platform mapping is not present for this platform, exiting";
    return std::nullopt;
  }

  // String value of profile id for printing in log
  std::string portProfileIdStr =
      apache::thrift::util::enumNameSafe(portProfileId);

  // Get port profile config for the given port profile id
  auto portProfileConfig = platformMapping_->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(portProfileId, PortID(portId)));
  if (!portProfileConfig.has_value()) {
    XLOG(INFO) << "For port profile id " << portProfileIdStr
               << ", the supported profile not found in platform mapping";
    return std::nullopt;
  }

  // Get the platform port entry for the given port id
  auto platformPortEntry = platformMapping_->getPlatformPorts().find(portId);
  if (platformPortEntry == platformMapping_->getPlatformPorts().end()) {
    XLOG(INFO) << "For port " << portId
               << ", the platform port not found in platform mapping";
    return std::nullopt;
  }

  // From the above platform port entry, get the port config for the given port
  // profile id
  auto platformPortConfig =
      platformPortEntry->second.supportedProfiles_ref()->find(portProfileId);
  if (platformPortConfig ==
      platformPortEntry->second.supportedProfiles_ref()->end()) {
    XLOG(INFO) << "For port id " << portId << " port profile id "
               << portProfileIdStr
               << ", the supported profile not found in platform mapping";
    return std::nullopt;
  }

  // Get the line polarity swap map
  auto linePolaritySwapMap = utility::getXphyLinePolaritySwapMap(
      *platformPortEntry->second.get_mapping().pins_ref(),
      platformMapping_->getChips());

  // Build the PhyPortConfig using platform port config pins list, polrity swap
  // map, port profile config
  phyPortConfig.config = phy::ExternalPhyConfig::fromConfigeratorTypes(
      *platformPortConfig->second.pins_ref(), linePolaritySwapMap);

  phyPortConfig.profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(*portProfileConfig);

  // Return true
  return phyPortConfig;
}

/*
 * getExternalPhyId
 *
 * This function takes the portId, port profile id and returns the external Phy
 * Id for that. This function looks into the platform mapping to derive the Phy
 * Id for a port and profile
 */
std::optional<uint32_t> WedgeManager::getExternalPhyId(int32_t portId) {
  auto platformPortEntry = platformMapping_->getPlatformPorts().find(portId);
  if (platformPortEntry == platformMapping_->getPlatformPorts().end()) {
    XLOG(INFO) << folly::sformat(
        "Platform port {:d} not found in platform mapping", portId);
    return std::nullopt;
  }

  const auto& chips = platformMapping_->getChips();
  if (chips.empty()) {
    throw FbossError(
        folly::sformat("Port{:d} Chip not found in platform mapping", portId));
  }

  const auto& xphy = utility::getDataPlanePhyChips(
      platformPortEntry->second, chips, phy::DataPlanePhyChipType::XPHY);

  if (xphy.size() != 1) {
    return std::nullopt;
  }

  return *xphy.begin()->second.physicalID_ref();
}

void WedgeManager::programXphyPort(
    int32_t portId,
    cfg::PortProfileID portProfileId) {
  if (phyManager_ == nullptr) {
    throw FbossError("Unable to program xphy port when PhyManager is not set");
  }
  phyManager_->programOnePort(PortID(portId), portProfileId);
}

} // namespace fboss
} // namespace facebook
