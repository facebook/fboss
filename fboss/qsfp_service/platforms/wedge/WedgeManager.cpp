// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/qsfp_service/QsfpConfig.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/sff/Sff8472Module.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"
#include "folly/futures/Future.h"

#include <fb303/ThreadCachedServiceData.h>

#include <folly/FileUtil.h>
#include <folly/gen/Base.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <chrono>

DEFINE_bool(
    override_program_iphy_ports_for_test,
    false,
    "Override wedge_agent programInternalPhyPorts(). For test only");

namespace facebook {
namespace fboss {

namespace {

constexpr int kSecAfterModuleOutOfReset = 2;

static const std::unordered_set<TransceiverID> kEmptryTransceiverIDs = {};

} // namespace

using LockedTransceiversPtr = folly::Synchronized<
    std::map<TransceiverID, std::unique_ptr<Transceiver>>>::WLockedPtr;

WedgeManager::WedgeManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping,
    PlatformType type)
    : TransceiverManager(std::move(api), std::move(platformMapping)),
      platformType_(type) {
  /* Constructor for WedgeManager class:
   * Get the TransceiverPlatformApi object from the creator of this object,
   * this object will be used for controlling the QSFP devices on board.
   * Going foward the qsfpPlatApi_ will be used to controll the QSFP devices
   * on FPGA managed platforms and the wedgeI2cBus_ will be used to control
   * the QSFP devices on I2C/CPLD managed platforms
   */
  if (FLAGS_publish_state_to_fsdb || FLAGS_publish_stats_to_fsdb) {
    fsdbSyncManager_ = std::make_unique<QsfpFsdbSyncManager>();
  }
}

WedgeManager::~WedgeManager() {
  if (fsdbSyncManager_) {
    fsdbSyncManager_->stop();
  }
}

void WedgeManager::loadConfig() {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  for (const auto& it : platformPorts) {
    auto port = it.second;
    // Get the transceiver id based on the port info from config.
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
    XLOG(INFO) << "Added port " << portName << " with portId " << portId
               << " to transceiver " << transceiverId.value();
  }

  // Process QSFP config here
  qsfpConfig_ = QsfpConfig::fromDefaultFile();
  if (FLAGS_publish_state_to_fsdb) {
    fsdbSyncManager_->updateConfig(qsfpConfig_->thrift);
    fsdbSyncManager_->start();
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

  // Check if a cold boot has been forced
  if (!canWarmBoot()) {
    XLOG(INFO) << "[COLD Boot] Will trigger all " << getNumQsfpModules()
               << " qsfps hard reset";
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
  }

  // Also try to load the config file here so that we have transceiver to port
  // mapping and port name recognization.
  loadConfig();

  agentConfig_ = AgentConfig::fromDefaultFile();

  // Set overrideTcvrToPortAndProfileForTest_ if
  // FLAGS_override_program_iphy_ports_for_test true.
  setOverrideTcvrToPortAndProfileForTesting();

  refreshTransceivers();
}

void WedgeManager::getTransceiversInfo(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiversInfo, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(*ids);
  }

  for (const auto& i : *ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is invalid, just skip and continue to the next.
      continue;
    }
    try {
      auto tcvrID = TransceiverID(i);
      info.insert({i, getTransceiverInfo(tcvrID)});
      auto currentState = getCurrentState(tcvrID);
      info[i].stateMachineState() = currentState;
      info[i].tcvrState()->stateMachineState() = currentState;
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Transceiver " << i
                << ": Error calling getTransceiverInfo(): " << ex.what();
    }
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
  auto ids = *(request->ids());
  XLOG(INFO) << "Received request for reading transceiver registers for ids: "
             << (ids.size() > 0 ? folly::join(",", ids) : "None");
  auto lockedTransceivers = transceivers_.rlock();

  std::vector<folly::Future<std::pair<int32_t, std::unique_ptr<IOBuf>>>>
      futResponses;
  for (const auto& i : ids) {
    // Initialize responses with valid = false. This will be overwritten with
    // the correct valid flag later
    responses[i].valid() = false;
    if (isValidTransceiver(i)) {
      if (auto it = lockedTransceivers->find(TransceiverID(i));
          it != lockedTransceivers->end()) {
        auto param = *(request->parameter());
        futResponses.push_back(it->second->futureReadTransceiver(param));
      }
    }
  }

  folly::collectAllUnsafe(futResponses)
      .thenValue([&responses](
                     const std::vector<folly::Try<
                         std::pair<int32_t, std::unique_ptr<IOBuf>>>>& tries) {
        for (const auto& tryResponse : tries) {
          if (tryResponse.hasValue()) {
            ReadResponse resp;
            auto tcvrId = tryResponse.value().first;
            resp.data() = *(tryResponse.value().second);
            resp.valid() = resp.data()->length() > 0;
            responses[tcvrId] = resp;
          } // We have already set valid to false above for all responses so
            // don't need to handle the exception here
        }
      })
      .wait();
}

void WedgeManager::writeTransceiverRegister(
    std::map<int32_t, WriteResponse>& responses,
    std::unique_ptr<WriteRequest> request) {
  auto ids = *(request->ids());
  XLOG(INFO) << "Received request for writing transceiver register for ids: "
             << (ids.size() > 0 ? folly::join(",", ids) : "None");
  auto lockedTransceivers = transceivers_.rlock();

  std::vector<folly::Future<std::pair<int32_t, bool>>> futResponses;

  for (const auto& i : ids) {
    // Initialize responses with success = false. This will be overwritten with
    // the correct success flag later
    responses[i].success() = false;

    if (isValidTransceiver(i)) {
      if (auto it = lockedTransceivers->find(TransceiverID(i));
          it != lockedTransceivers->end()) {
        auto param = *(request->parameter());
        futResponses.push_back(
            it->second->futureWriteTransceiver(param, *(request->data())));
      }
    }
  }

  folly::collectAllUnsafe(futResponses)
      .thenValue(
          [&responses](
              const std::vector<folly::Try<std::pair<int32_t, bool>>>& tries) {
            for (const auto& tryResponse : tries) {
              if (tryResponse.hasValue()) {
                WriteResponse resp;
                auto tcvrId = tryResponse.value().first;
                resp.success() = tryResponse.value().second;
                responses[tcvrId] = resp;
              } // We have already set success to false above for all responses
                // so don't need to handle the exception here
            }
          })
      .wait();
}

void WedgeManager::customizeTransceiver(int32_t idx, cfg::PortSpeed speed) {
  if (!isValidTransceiver(idx)) {
    return;
  }
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(TransceiverID(idx));
      it != lockedTransceivers->end()) {
    try {
      auto portName = getPortName(TransceiverID(idx));
      // This API uses transceiverID so we don't know which port to program.
      // Just program the first port
      TransceiverPortState state{portName, 0, speed};
      it->second->customizeTransceiver(state);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Transceiver " << idx
                << ": Error calling customizeTransceiver(): " << ex.what();
    }
  }
}

void WedgeManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  std::set<TransceiverID> tcvrIDs;
  for (const auto& [portID, portStatus] : *ports) {
    if (auto tcvrIdx = portStatus.transceiverIdx()) {
      tcvrIDs.insert(TransceiverID(*tcvrIdx->transceiverId()));
    }
  }
  // Update Transceiver active state
  updateTransceiverActiveState(tcvrIDs, *ports);
  // Only fetch the transceivers for the input ports
  for (auto tcvrID : tcvrIDs) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto it = lockedTransceivers->find(tcvrID);
        it != lockedTransceivers->end()) {
      try {
        info[tcvrID] = it->second->getTransceiverInfo();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Transceiver " << tcvrID
                  << ": Error calling getTransceiverInfo(): " << ex.what();
      }
    }
  }
}

// NOTE: this may refresh transceivers multiple times if they're newly plugged
//  in, as refresh() is called both via updateTransceiverMap and futureRefresh
std::vector<TransceiverID> WedgeManager::refreshTransceivers() {
  try {
    wedgeI2cBus_->verifyBus(false);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error calling verifyBus(): " << ex.what();
    return {};
  }

  clearAllTransceiverReset();

  // Since transceivers may appear or disappear, we need to update our
  // transceiver mapping and type here.
  updateTransceiverMap();

  // Finally refresh all transceivers without specifying any ids
  return TransceiverManager::refreshTransceivers(kEmptryTransceiverIDs);
}

void WedgeManager::publishTransceiversToFsdb(
    const std::vector<TransceiverID>& ids) {
  if (!FLAGS_publish_stats_to_fsdb) {
    return;
  }

  TcvrInfoMap tcvrInfos;
  getTransceiversInfo(tcvrInfos, std::make_unique<std::vector<int32_t>>());

  // Publish states on refreshed transceivers
  for (int32_t id : ids.empty() ? folly::gen::range(0, getNumQsfpModules()) |
               folly::gen::eachAs<TransceiverID>() |
               folly::gen::as<std::vector<TransceiverID>>()
                                : ids) {
    auto iter = tcvrInfos.find(id);
    if (iter == tcvrInfos.end()) {
      continue;
    }
    fsdbSyncManager_->updateTcvrState(id, std::move(*iter->second.tcvrState()));
  }

  // Publish all stats (No deltas. Just publish the best we have for all
  // transceivers.)
  QsfpFsdbSyncManager::TcvrStatsMap stats;
  for (const auto& [id, info] : tcvrInfos) {
    stats[id] = *info.tcvrStats();
  }
  fsdbSyncManager_->updateTcvrStats(std::move(stats));
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
  XLOG(INFO) << "triggerQsfpHardReset called for " << idx;
  qsfpPlatApi_->triggerQsfpHardReset(idx + 1);
  bool removeTransceiver = false;
  {
    // Read Lock to trigger all state machine changes
    auto lockedTransceivers = transceivers_.rlock();
    if (auto it = lockedTransceivers->find(TransceiverID(idx));
        it != lockedTransceivers->end()) {
      it->second->removeTransceiver();
      removeTransceiver = true;
    }
  }

  if (removeTransceiver) {
    // Write lock to remove the transceiver
    auto lockedTransceivers = transceivers_.wlock();
    auto it = lockedTransceivers->find(TransceiverID(idx));
    lockedTransceivers->erase(it);
  }
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

  std::unordered_set<int> tcvrsToCreate;
  std::unordered_set<int> tcvrsToDelete;
  std::unordered_set<int> tcvrsToHardReset;

  {
    auto lockedTransceiversRPtr = transceivers_.rlock();
    for (int idx = 0; idx < qsfpImpls.size(); idx++) {
      if (!futInterfaces[idx].isReady()) {
        XLOG(ERR)
            << "Failed getting TransceiverManagementInterface for TransceiverID="
            << idx;
        continue;
      }
      auto it = lockedTransceiversRPtr->find(TransceiverID(idx));
      if (it != lockedTransceiversRPtr->end()) {
        // In the case where we already have a transceiver recorded, try to
        // check whether they match the transceiver type.
        if (it->second->managementInterface() == futInterfaces[idx].value()) {
          // The management interface matches. Nothing needs to be done.
          continue;
        } else {
          // The management changes. Need to Delete the old module to make place
          // for the new one.
          it->second->removeTransceiver();
          tcvrsToDelete.insert(idx);
        }
      }

      // Either we don't have a transceiver here before or we had a new one
      // since the management interface changed, we want to create a new module
      // here.
      tcvrsToCreate.insert(idx);
    }
  } // end of scope for transceivers_.rlock

  {
    auto lockedTransceiversWPtr = transceivers_.wlock();
    // Delete the transceivers first before potentially creating them later
    for (auto idx : tcvrsToDelete) {
      auto it = lockedTransceiversWPtr->find(TransceiverID(idx));
      lockedTransceiversWPtr->erase(it);
    }

    for (auto idx : tcvrsToCreate) {
      if (futInterfaces[idx].value() == TransceiverManagementInterface::CMIS) {
        XLOG(INFO) << "Making CMIS QSFP for TransceiverID=" << idx;
        lockedTransceiversWPtr->emplace(
            TransceiverID(idx),
            std::make_unique<CmisModule>(this, std::move(qsfpImpls[idx])));
      } else if (
          futInterfaces[idx].value() == TransceiverManagementInterface::SFF) {
        XLOG(INFO) << "Making Sff QSFP for TransceiverID=" << idx;
        lockedTransceiversWPtr->emplace(
            TransceiverID(idx),
            std::make_unique<SffModule>(this, std::move(qsfpImpls[idx])));
      } else if (
          futInterfaces[idx].value() ==
          TransceiverManagementInterface::SFF8472) {
        XLOG(INFO) << "Making Sff8472 module for TransceiverID=" << idx;
        lockedTransceiversWPtr->emplace(
            TransceiverID(idx),
            std::make_unique<Sff8472Module>(this, std::move(qsfpImpls[idx])));
      } else {
        XLOG(ERR) << "Unknown Transceiver interface: "
                  << static_cast<int>(futInterfaces[idx].value())
                  << " for TransceiverID=" << idx;

        try {
          if (!qsfpImpls[idx]->detectTransceiver()) {
            XLOG(DBG3) << "Transceiver is not present. TransceiverID=" << idx;
            continue;
          }
        } catch (const std::exception& ex) {
          XLOG(ERR) << "Failed to detect transceiver. TransceiverID=" << idx
                    << ": " << ex.what();
          continue;
        }

        // There are times when a module cannot be read however it's present.
        // Try to reset here since that may be able to bring it back.
        // Check if we have expected ports info synced over and if all of
        // the ports are down. If any of them is not down then we will not
        // perform the reset.
        bool safeToReset = areAllPortsDown(TransceiverID(idx)).first;
        if (std::time(nullptr) <= pauseRemediationUntil_) {
          XLOG(WARN) << "Remediation is paused, won't hard reset a present "
                     << "transceiver with unknown interface. TransceiverID="
                     << idx;
        } else if (safeToReset) {
          tcvrsToHardReset.insert(idx);
        } else {
          XLOG(ERR) << "Unknown interface of transceiver with ports up at "
                    << idx;
        }
      }
    } // end of scope for transceivers_.wlock
  }

  for (auto idx : tcvrsToHardReset) {
    try {
      XLOG(INFO) << "Try hard reset a present transceiver with unknown "
                 << "interface. TransceiverID=" << idx;
      triggerQsfpHardReset(idx);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Failed to triggerQsfpHardReset for TransceiverID=" << idx
                << ": " << ex.what();
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
        "qsfp.", *counter.controllerName_(), ".readTotal");
    tcData().setCounter(statName, *counter.readTotal_());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName_(), ".readFailed");
    tcData().setCounter(statName, *counter.readFailed_());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName_(), ".readBytes");
    tcData().setCounter(statName, *counter.readBytes_());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName_(), ".writeTotal");
    tcData().setCounter(statName, *counter.writeTotal_());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName_(), ".writeFailed");
    tcData().setCounter(statName, *counter.writeFailed_());

    statName = folly::to<std::string>(
        "qsfp.", *counter.controllerName_(), ".writeBytes");
    tcData().setCounter(statName, *counter.writeBytes_());
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
 * triggerVdmStatsCapture
 *
 * This function triggers the next VDM data capture for the list of
 * transceiver Id's to be displayed in ODS
 */
void WedgeManager::triggerVdmStatsCapture(std::vector<int32_t>& ids) {
  XLOG(DBG2) << "triggerVdmStatsCapture, with ids: "
             << (ids.size() > 0 ? folly::join(",", ids) : "None");
  if (ids.empty()) {
    folly::gen::range(0, getNumQsfpModules()) | folly::gen::appendTo(ids);
  }

  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& i : ids) {
    if (!isValidTransceiver(i)) {
      // If the transceiver idx is not valid,
      // just skip and continue to the next.
      continue;
    }
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      // Calling the trigger VDM stats capure function for transceiver
      try {
        it->second->triggerVdmStatsCapture();
      } catch (std::exception& e) {
        XLOG(ERR) << "Transceiver VDM could not be reset for port "
                  << TransceiverID(i) << " message: " << e.what();
        continue;
      }
    }
  }
}

void WedgeManager::getAndClearTransceiversModuleStatus(
    std::map<int32_t, ModuleStatus>& moduleStatusMap,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "getAndClearTransceiversModuleStatus, with ids: "
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
    if (auto it = lockedTransceivers->find(TransceiverID(i));
        it != lockedTransceivers->end()) {
      moduleStatusMap[i] = it->second->readAndClearCachedModuleStatus();
    }
  }
}

phy::PhyInfo WedgeManager::getXphyInfo(PortID portID) {
  if (phyManager_ == nullptr) {
    throw FbossError("Unable to get xphy info when PhyManager is not set");
  }

  if (auto phyInfoOpt = phyManager_->getXphyInfo(portID)) {
    return *phyInfoOpt;
  } else {
    throw FbossError("Unable to get xphy info for port: ", portID);
  }
}

// This function is used to handle thrift requests from clients to test
// programming a xphy port on demand and used in the hw tests
void WedgeManager::programXphyPort(
    PortID portId,
    cfg::PortProfileID portProfileId) {
  if (phyManager_ == nullptr) {
    throw FbossError("Unable to program xphy port when PhyManager is not set");
  }

  std::optional<TransceiverInfo> itTcvr;
  // Get the transceiver id for the given port id
  if (auto tcvrID = getTransceiverID(portId)) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto it = lockedTransceivers->find(*tcvrID);
        it != lockedTransceivers->end()) {
      itTcvr = it->second->getTransceiverInfo();
    } else {
      XLOG(WARNING) << "Port:" << portId
                    << " doesn't have transceiver info for transceiver id:"
                    << *tcvrID;
    }
  }

  phyManager_->programOnePort(
      portId, portProfileId, itTcvr, false /* needResetDataPath */);
}

bool WedgeManager::initExternalPhyMap(bool forceWarmboot) {
  if (!phyManager_) {
    // If there's no PhyManager for such platform, skip init xphy map
    return true;
  }

  // First call PhyManager::initExternalPhyMap() to create xphy map
  auto rb = phyManager_->initExternalPhyMap();
  // TODO(ccpowers): We could probably clean this up a bit to separate out the
  // warmboot file logic and the phy init logic, to make it easier to
  // init phys from external CLIs

  // forceWarmboot is only used to skip checking the warmboot file
  // when we're running outside of qsfp_service (e.g. in the fboss-xphy CLI)
  bool warmboot = forceWarmboot || canWarmBoot();

  // And then initialize the xphy for each pim which has xphy
  std::vector<folly::Future<folly::Unit>> initPimTasks;
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();
  for (int pimIndex = 0; pimIndex < phyManager_->getNumOfSlot(); ++pimIndex) {
    auto pimID = PimID(pimIndex + phyManager_->getPimStartNum());
    if (!phyManager_->shouldInitializePimXphy(pimID)) {
      XLOG(WARN) << "Skip intializing pim xphy for pim="
                 << static_cast<int>(pimID);
      continue;
    }
    XLOG(DBG1) << "[" << (warmboot ? "WARM" : "COLD")
               << " Boot] Initializing PIM " << static_cast<int>(pimID);
    auto* pimEventBase = phyManager_->getPimEventBase(pimID);
    initPimTasks.push_back(
        folly::via(pimEventBase)
            .thenValue([&, pimID, warmboot](auto&&) {
              phyManager_->initializeSlotPhys(pimID, warmboot);
            })
            .thenError(
                folly::tag_t<std::exception>{},
                [pimID](const std::exception& e) {
                  XLOG(WARNING) << "Exception in initializeSlotPhys() for pim="
                                << static_cast<int>(pimID) << ", "
                                << folly::exceptionStr(e);
                }));
  }

  if (!initPimTasks.empty()) {
    folly::collectAll(initPimTasks).wait();
    XLOG(DBG2) << "Initialized all pims xphy took "
               << std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::steady_clock::now() - begin)
                      .count()
               << " seconds";

    // Restore warm boot Phy state after initializing all the phys.
    restoreWarmBootPhyState();
  }
  return rb;
}

void WedgeManager::programXphyPortPrbs(
    PortID portID,
    phy::Side side,
    const phy::PortPrbsState& prbs) {
  phyManager_->setPortPrbs(portID, side, prbs);
}

phy::PortPrbsState WedgeManager::getXphyPortPrbs(
    PortID portID,
    phy::Side side) {
  return phyManager_->getPortPrbs(portID, side);
}

void WedgeManager::updateAllXphyPortsStats() {
  if (!phyManager_) {
    // If there's no PhyManager for such platform, skip updating xphy stats
    return;
  }
  // Then we need to update all the programmed port xphy stats
  phyManager_->updateAllXphyPortsStats();
}

std::vector<PortID> WedgeManager::getMacsecCapablePorts() const {
  if (!phyManager_) {
    return {};
  }
  return phyManager_->getPortsSupportingFeature(
      phy::ExternalPhy::Feature::MACSEC);
}

void WedgeManager::setOverrideTcvrToPortAndProfileForTesting(
    std::optional<OverrideTcvrToPortAndProfile> overrideTcvrToPortAndProfile) {
  if (overrideTcvrToPortAndProfile) {
    overrideTcvrToPortAndProfileForTest_ = *overrideTcvrToPortAndProfile;
  } else if (FLAGS_override_program_iphy_ports_for_test) {
    const auto& chips = platformMapping_->getChips();
    for (auto chip : chips) {
      if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto tcvrID = TransceiverID(*chip.second.physicalID());
      overrideTcvrToPortAndProfileForTest_[tcvrID] = {};
    }
    // Use QSFP config to get the iphy port and profile
    auto qsfpTestConfig = qsfpConfig_->thrift.qsfpTestConfig();
    CHECK(qsfpTestConfig.has_value());
    for (const auto& portPairs : *qsfpTestConfig->cabledPortPairs()) {
      auto aPortID = getPortIDByPortName(*portPairs.aPortName());
      auto zPortID = getPortIDByPortName(*portPairs.zPortName());
      CHECK(aPortID.has_value());
      CHECK(zPortID.has_value());
      // If the SW port has transceiver id, add it to
      // overrideTcvrToPortAndProfile
      if (auto tcvrID = getTransceiverID(PortID(*aPortID))) {
        overrideTcvrToPortAndProfileForTest_[*tcvrID].emplace(
            *aPortID, *portPairs.profileID());
      }
      if (auto tcvrID = getTransceiverID(PortID(*zPortID))) {
        overrideTcvrToPortAndProfileForTest_[*tcvrID].emplace(
            *zPortID, *portPairs.profileID());
      }
    }
  }
}

std::string WedgeManager::listHwObjects(
    std::vector<HwObjectType>& hwObjects,
    bool cached) const {
  if (!phyManager_) {
    return "";
  }
  return phyManager_->listHwObjects(hwObjects, cached);
}

bool WedgeManager::getSdkState(std::string filename) const {
  if (!phyManager_) {
    return false;
  }
  return phyManager_->getSdkState(filename);
}

void WedgeManager::publishPhyStateToFsdb(
    std::string&& portName,
    std::optional<phy::PhyState>&& newState) const {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbSyncManager_->updatePhyState(std::move(portName), std::move(newState));
  }
}

void WedgeManager::publishPhyStatToFsdb(
    std::string&& portName,
    phy::PhyStats&& stat) const {
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbSyncManager_->updatePhyStat(std::move(portName), std::move(stat));
  }
}

} // namespace fboss
} // namespace facebook
