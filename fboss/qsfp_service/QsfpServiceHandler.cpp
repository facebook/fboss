// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"

#include <fboss/lib/LogThriftCall.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

DEFINE_string(
    sak_list_warmboot_config,
    "/var/facebook/fboss/mka_service/sak_lists/",
    "path to store the physervice SAK config for service restart");
DEFINE_string(
    sak_list_warmboot_filename,
    "sak_config",
    "filename of warmbootconfig");

DEFINE_int64(
    sak_config_validity_in_secs,
    3600,
    "warmboot config validity in secs");

DEFINE_int32(
    phy_service_macsec_port,
    5910,
    "Port for the phy service thrift service");

namespace facebook {
namespace fboss {

template <typename Type>
static void valid(const Type& val) {
  if (!val) {
    throw FbossError("Invalid input");
  }
}

QsfpServiceHandler::QsfpServiceHandler(
    std::unique_ptr<TransceiverManager> manager,
    std::shared_ptr<mka::MacsecHandler> handler)
    : ::facebook::fb303::FacebookBase2DeprecationMigration("QsfpService"),
      manager_(std::move(manager)),
      macsecHandler_(handler) {
  XLOG(INFO) << "FbossPhyMacsecService inside QsfpServiceHandler Started";
}

QsfpServiceHandler::~QsfpServiceHandler() {
  if (fsdbSubscriber_) {
    fsdbSubscriber_->removeSwitchStatePortMapSubscription();
  }
}

void QsfpServiceHandler::init() {
  manager_->init();
  if (FLAGS_subscribe_to_state_from_fsdb) {
    fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>("qsfp_service");
    fsdbSubscriber_ =
        std::make_unique<QsfpFsdbSubscriber>(fsdbPubSubMgr_.get());
    fsdbSubscriber_->subscribeToSwitchStatePortMap(manager_.get());
  }
}

facebook::fb303::cpp2::fb_status QsfpServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

TransceiverType QsfpServiceHandler::getType(int32_t /* unused */) {
  auto log = LOG_THRIFT_CALL(INFO);
  return TransceiverType::QSFP;
}

void QsfpServiceHandler::getTransceiverInfo(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversInfo(info, std::move(ids));
}

void QsfpServiceHandler::getPortsRequiringOpticsFwUpgrade(
    std::map<std::string, FirmwareUpgradeData>& ports) {
  auto log = LOG_THRIFT_CALL(INFO);
  ports = manager_->getPortsRequiringOpticsFwUpgrade();
}

void QsfpServiceHandler::triggerAllOpticsFwUpgrade(
    std::map<std::string, FirmwareUpgradeData>& ports) {
  auto log = LOG_THRIFT_CALL(INFO);
  ports = manager_->triggerAllOpticsFwUpgrade();
}

void QsfpServiceHandler::getTransceiverConfigValidationInfo(
    std::map<int32_t, std::string>& info,
    std::unique_ptr<std::vector<int32_t>> ids,
    bool getConfigString) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getAllTransceiversValidationInfo(
      info, std::move(ids), getConfigString);
}

void QsfpServiceHandler::getTransceiverRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversRawDOMData(info, std::move(ids));
}

void QsfpServiceHandler::getTransceiverDOMDataUnion(
    std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversDOMDataUnion(info, std::move(ids));
}

void QsfpServiceHandler::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->syncPorts(info, std::move(ports));
}

void QsfpServiceHandler::resetTransceiver(
    std::unique_ptr<std::vector<std::string>> portNames,
    ResetType resetType,
    ResetAction resetAction) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->resetTransceiver(std::move(portNames), resetType, resetAction);
}

void QsfpServiceHandler::pauseRemediation(
    int32_t timeout,
    std::unique_ptr<std::vector<std::string>> portList) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPauseRemediation(timeout, std::move(portList));
}

void QsfpServiceHandler::unpauseRemediation(
    std::unique_ptr<std::vector<std::string>> portList) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPauseRemediation(0, std::move(portList));
}

void QsfpServiceHandler::getRemediationUntilTime(
    std::map<std::string, int32_t>& info,
    std::unique_ptr<std::vector<std::string>> portList) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getPauseRemediationUntil(info, std::move(portList));
}

void QsfpServiceHandler::getSymbolErrorHistogram(
    CdbDatapathSymErrHistogram& symErr,
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getSymbolErrorHistogram(symErr, *portName);
}

void QsfpServiceHandler::getAllPortSupportedProfiles(
    std::map<std::string, std::vector<cfg::PortProfileID>>&
        supportedPortProfiles,
    bool checkOptics) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getAllPortSupportedProfiles(supportedPortProfiles, checkOptics);
}

void QsfpServiceHandler::readTransceiverRegister(
    std::map<int32_t, ReadResponse>& response,
    std::unique_ptr<ReadRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto param = *(request->parameter());
  auto offset = *(param.offset());
  if (offset < 0 || offset > 255) {
    throw FbossError("Offset cannot be < 0 or > 255");
  }
  auto page_ref = param.page();
  if (page_ref.has_value() && *page_ref < 0) {
    throw FbossError("Page cannot be < 0");
  }
  auto length_ref = param.length();
  if (length_ref.has_value()) {
    if (*length_ref < 0 || *length_ref > 255) {
      throw FbossError("Length cannot be < 0 or > 255");
    } else if (*length_ref + offset > 256) {
      throw FbossError("Offset + Length cannot be > 256");
    }
  }
  manager_->readTransceiverRegister(response, std::move(request));
}

void QsfpServiceHandler::writeTransceiverRegister(
    std::map<int32_t, WriteResponse>& response,
    std::unique_ptr<WriteRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto param = *(request->parameter());
  auto offset = *(param.offset());
  if (offset < 0 || offset > 255) {
    throw FbossError("Offset cannot be < 0 or > 255");
  }
  auto page_ref = param.page();
  if (page_ref.has_value() && *page_ref < 0) {
    throw FbossError("Page cannot be < 0");
  }
  manager_->writeTransceiverRegister(response, std::move(request));
}

QsfpServiceRunState QsfpServiceHandler::getQsfpServiceRunState() {
  auto log = LOG_THRIFT_CALL(INFO);
  return manager_->getRunState();
}

void QsfpServiceHandler::programXphyPort(
    int32_t portId,
    cfg::PortProfileID portProfileId) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->programXphyPort(PortID(portId), portProfileId);
}

void QsfpServiceHandler::getXphyInfo(phy::PhyInfo& response, int32_t portID) {
  auto log = LOG_THRIFT_CALL(INFO);
  response = manager_->getXphyInfo(PortID(portID));
}

void QsfpServiceHandler::getSupportedPrbsPolynomials(
    std::vector<prbs::PrbsPolynomial>& prbsCapabilities,
    std::unique_ptr<std::string> portName,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getSupportedPrbsPolynomials(prbsCapabilities, *portName, component);
}

void QsfpServiceHandler::setInterfacePrbs(
    std::unique_ptr<std::string> portName,
    phy::PortComponent component,
    std::unique_ptr<prbs::InterfacePrbsState> state) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setInterfacePrbs(*portName, component, *state);
}

void QsfpServiceHandler::getInterfacePrbsState(
    prbs::InterfacePrbsState& prbsState,
    std::unique_ptr<std::string> portName,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getInterfacePrbsState(prbsState, *portName, component);
}

void QsfpServiceHandler::getAllInterfacePrbsStates(
    std::map<std::string, prbs::InterfacePrbsState>& prbsStates,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getAllInterfacePrbsStates(prbsStates, component);
}

void QsfpServiceHandler::getInterfacePrbsStats(
    phy::PrbsStats& response,
    std::unique_ptr<std::string> portName,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  response = manager_->getInterfacePrbsStats(*portName, component);
}

void QsfpServiceHandler::getAllInterfacePrbsStats(
    std::map<std::string, phy::PrbsStats>& prbsStats,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getAllInterfacePrbsStats(prbsStats, component);
}

void QsfpServiceHandler::clearInterfacePrbsStats(
    std::unique_ptr<std::string> portName,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->clearInterfacePrbsStats(*portName, component);
}

void QsfpServiceHandler::bulkClearInterfacePrbsStats(
    std::unique_ptr<std::vector<std::string>> interfaces,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->bulkClearInterfacePrbsStats(std::move(interfaces), component);
}

void QsfpServiceHandler::dumpTransceiverI2cLog(
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto ret = manager_->dumpTransceiverI2cLog(*portName);
  // if the header of the log has size 0, logging is not enabled.
  if (ret.first == 0) {
    throw FbossError(
        fmt::format("Failed to dump transceiver {} I2c log", *portName));
  }
}

void QsfpServiceHandler::setPortPrbs(
    int32_t portId,
    phy::PortComponent component,
    std::unique_ptr<phy::PortPrbsState> state) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPortPrbs(PortID(portId), component, *state);
}

void QsfpServiceHandler::getPortPrbsStats(
    phy::PrbsStats& response,
    int32_t portId,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  response = manager_->getPortPrbsStats(PortID(portId), component);
}

void QsfpServiceHandler::clearPortPrbsStats(
    int32_t portId,
    phy::PortComponent component) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->clearPortPrbsStats(PortID(portId), component);
}

void QsfpServiceHandler::getMacsecCapablePorts(std::vector<int32_t>& ports) {
  auto macsecPorts = manager_->getMacsecCapablePorts();
  std::for_each(macsecPorts.begin(), macsecPorts.end(), [&ports](auto portId) {
    ports.push_back(static_cast<int32_t>(portId));
  });
}

void QsfpServiceHandler::validateHandler() const {
  if (!macsecHandler_) {
    throw FbossError("Macsec handler not initialized");
  }
}

void QsfpServiceHandler::listHwObjects(
    std::string& out,
    std::unique_ptr<std::vector<HwObjectType>> hwObjects,
    bool cached) {
  auto log = LOG_THRIFT_CALL(INFO);
  out = manager_->listHwObjects(*hwObjects, cached);
}

bool QsfpServiceHandler::getSdkState(std::unique_ptr<std::string> fileName) {
  auto log = LOG_THRIFT_CALL(INFO);
  return manager_->getSdkState(*fileName);
}

void QsfpServiceHandler::getPortInfo(
    std::string& out,
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  out = manager_->getPortInfo(*portName);
}

void QsfpServiceHandler::setPortLoopbackState(
    std::unique_ptr<std::string> portName,
    phy::PortComponent component,
    bool setLoopback) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPortLoopbackState(*portName, component, setLoopback);
}

void QsfpServiceHandler::setPortAdminState(
    std::unique_ptr<std::string> portName,
    phy::PortComponent component,
    bool setAdminUp) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPortAdminState(*portName, component, setAdminUp);
}

void QsfpServiceHandler::setInterfaceTxRx(
    std::vector<phy::TxRxEnableResponse>& txRxEnableResponse,
    std::unique_ptr<std::vector<phy::TxRxEnableRequest>> txRxEnableRequests) {
  auto log = LOG_THRIFT_CALL(INFO);
  txRxEnableResponse = manager_->setInterfaceTxRx(*txRxEnableRequests);
}

void QsfpServiceHandler::saiPhyRegisterAccess(
    std::string& out,
    std::unique_ptr<std::string> portName,
    bool opRead,
    int phyAddr,
    int devId,
    int regOffset,
    int data) {
  auto log = LOG_THRIFT_CALL(INFO);
  out = manager_->saiPhyRegisterAccess(
      *portName, opRead, phyAddr, devId, regOffset, data);
}

void QsfpServiceHandler::saiPhySerdesRegisterAccess(
    std::string& out,
    std::unique_ptr<std::string> portName,
    bool opRead,
    int16_t mdioAddr,
    phy::Side side,
    int serdesLane,
    int64_t regOffset,
    int64_t data) {
  auto log = LOG_THRIFT_CALL(INFO);
  out = manager_->saiPhySerdesRegisterAccess(
      *portName, opRead, mdioAddr, side, serdesLane, regOffset, data);
}

void QsfpServiceHandler::phyConfigCheckHw(
    std::string& out,
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  out = manager_->phyConfigCheckHw(*portName);
}

void QsfpServiceHandler::publishLinkSnapshots(
    std::unique_ptr<std::vector<std::string>> portNames) {
  auto log = LOG_THRIFT_CALL(INFO);
  for (const auto& portName : *portNames) {
    manager_->publishLinkSnapshots(portName);
  }
}

void QsfpServiceHandler::getAllInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getAllInterfacePhyInfo(phyInfos);
}

void QsfpServiceHandler::getInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos,
    std::unique_ptr<std::vector<std::string>> portNames) {
  auto log = LOG_THRIFT_CALL(INFO);
  for (const auto& portName : *portNames) {
    manager_->getInterfacePhyInfo(phyInfos, portName);
  }
}

#if FOLLY_HAS_COROUTINES

folly::coro::Task<bool> QsfpServiceHandler::co_sakInstallRx(
    std::unique_ptr<mka::MKASak> sak,
    std::unique_ptr<mka::MKASci> sciToAdd) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  valid<decltype(sak)>(sak);
  valid<decltype(sciToAdd)>(sciToAdd);
  bool ret = macsecHandler_->sakInstallRx(*sak, *sciToAdd);
  /* TODO(rajank): Move/enable this for mka warmboot support
  if (ret) {
    mka::MKAActiveSakSession sess;
    sess.sak_ref() = std::move(*sak);
    sess.sciList_ref()->emplace_back(std::move(*sciToAdd));
    sakSet_.items.emplace(std::move(sess));
    updateWarmBootConfig();
  }
  */
  co_return ret;
}

folly::coro::Task<bool> QsfpServiceHandler::co_sakInstallTx(
    std::unique_ptr<mka::MKASak> sak) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  valid<decltype(sak)>(sak);
  bool ret = macsecHandler_->sakInstallTx(*sak);
  /* TODO(rajank): Move/enable this for mka warmboot support
  if (ret) {
    updateWarmBootConfig();
  }
  */
  co_return ret;
}

folly::coro::Task<bool> QsfpServiceHandler::co_sakDeleteRx(
    std::unique_ptr<mka::MKASak> sak,
    std::unique_ptr<mka::MKASci> sciToRemove) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  valid<decltype(sak)>(sak);
  valid<decltype(sciToRemove)>(sciToRemove);
  bool ret = macsecHandler_->sakDeleteRx(*sak, *sciToRemove);
  /* TODO(rajank): Move/enable this for mka warmboot support
  if (ret) {
    mka::MKAActiveSakSession sess;
    sess.sak_ref() = std::move(*sak);
    std::unordered_set<mka::MKAActiveSakSession>::iterator iter =
        sakSet_.items.find(sess);
    if (iter == sakSet_.items.end()) {
      co_return ret;
    }
    // Expected copy. The unordered set values are const and can't be
    // modified, so we copy and replace it.
    std::vector<mka::MKASci> list = *iter->sciList_ref();
    for (auto sciIter = list.begin(); sciIter != list.end(); sciIter++) {
      auto& sci = *sciIter;
      if (*sci.macAddress_ref() == *sciToRemove->macAddress_ref() &&
          *sci.port_ref() == *sciToRemove->port_ref()) {
        list.erase(sciIter);
        break;
      }
    }
    sess.sciList_ref() = std::move(list);
    sakSet_.items.erase(iter);
    sakSet_.items.emplace(sess);
    updateWarmBootConfig();
  }
  */
  co_return ret;
}

folly::coro::Task<bool> QsfpServiceHandler::co_sakDelete(
    std::unique_ptr<mka::MKASak> sak) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  valid<decltype(sak)>(sak);
  bool ret = macsecHandler_->sakDelete(*sak);
  /* TODO(rajank): Move/enable this for mka warmboot support
  if (ret) {
    mka::MKAActiveSakSession sess;
    sess.sak_ref() = std::move(*sak);
    sakSet_.items.erase(sess);
    updateWarmBootConfig();
  }
  */
  co_return ret;
}

folly::coro::Task<std::unique_ptr<mka::MKASakHealthResponse>>
QsfpServiceHandler::co_sakHealthCheck(std::unique_ptr<mka::MKASak> sak) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  valid<decltype(sak)>(sak);
  // TODO:(shankaran) - when healthcheck fails remove the sak from set.
  co_return std::make_unique<mka::MKASakHealthResponse>(
      macsecHandler_->sakHealthCheck(*sak));
}

folly::coro::Task<std::unique_ptr<mka::MacsecPortPhyMap>>
QsfpServiceHandler::co_macsecGetPhyPortInfo(
    std::unique_ptr<std::vector<std::string>> portNames) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return std::make_unique<mka::MacsecPortPhyMap>(
      macsecHandler_->macsecGetPhyPortInfo(*portNames));
}

folly::coro::Task<PortOperState> QsfpServiceHandler::co_macsecGetPhyLinkInfo(
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return macsecHandler_->macsecGetPhyLinkInfo(*portName);
}

folly::coro::Task<std::unique_ptr<phy::PhyInfo>>
QsfpServiceHandler::co_getPhyInfo(std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return std::make_unique<phy::PhyInfo>(
      macsecHandler_->getPhyInfo(*portName));
}

folly::coro::Task<bool> QsfpServiceHandler::co_deleteAllSc(
    std::unique_ptr<std::string> portName) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return macsecHandler_->deleteAllSc(*portName);
}

folly::coro::Task<bool> QsfpServiceHandler::co_setupMacsecState(
    std::unique_ptr<std::vector<std::string>> portList,
    bool macsecDesired,
    bool dropUnencrypted) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return macsecHandler_->setupMacsecState(
      *portList, macsecDesired, dropUnencrypted);
}

folly::coro::Task<std::unique_ptr<std::map<std::string, MacsecStats>>>
QsfpServiceHandler::co_getAllMacsecPortStats(bool readFromHw) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return std::make_unique<std::map<std::string, MacsecStats>>(
      macsecHandler_->getAllMacsecPortStats(readFromHw));
}

folly::coro::Task<std::unique_ptr<std::map<std::string, MacsecStats>>>
QsfpServiceHandler::co_getMacsecPortStats(
    std::unique_ptr<std::vector<std::string>> portNames,
    bool readFromHw) {
  auto log = LOG_THRIFT_CALL(INFO);
  validateHandler();
  co_return std::make_unique<std::map<std::string, MacsecStats>>(
      macsecHandler_->getMacsecPortStats(*portNames, readFromHw));
}
#endif

} // namespace fboss
} // namespace facebook
