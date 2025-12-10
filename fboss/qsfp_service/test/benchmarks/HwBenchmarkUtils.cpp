// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

std::vector<TransceiverID> getMatchingTcvrIds(
    const std::shared_ptr<WedgeManager>& wedgeMgr,
    MediaInterfaceCode mediaType) {
  std::vector<TransceiverID> tcvrIds;
  auto qsfpConfig = wedgeMgr->getQsfpConfig();
  if (qsfpConfig == nullptr) {
    throw FbossError("Qsfp Config is not set.");
  }
  auto qsfpTestConfig = qsfpConfig->thrift.qsfpTestConfig();
  CHECK(qsfpTestConfig.has_value());

  for (const auto& portPairs : *qsfpTestConfig->cabledPortPairs()) {
    for (auto currentPortName :
         {portPairs.aPortName(), portPairs.zPortName()}) {
      auto currentPortID = wedgeMgr->getPortIDByPortName(*currentPortName);
      CHECK(currentPortID.has_value());
      auto tcvrID = wedgeMgr->getTransceiverID(PortID(*currentPortID));
      if (!tcvrID) {
        continue;
      }

      auto interface = wedgeMgr->getTransceiverInfo(*tcvrID)
                           .tcvrState()
                           ->moduleMediaInterface();

      if (interface.has_value() && interface.value() == mediaType) {
        tcvrIds.push_back(*tcvrID);
      }
    }
  }

  return tcvrIds;
}

std::size_t refreshTcvrs(MediaInterfaceCode mediaType) {
  // Initialization
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);
  folly::BenchmarkSuspender suspender;
  // Making shared ptr so that we can use common helper function.
  std::shared_ptr<WedgeManager> wedgeMgr;
  std::unique_ptr<PortManager> portMgr;
  std::tie(wedgeMgr, portMgr) = setupForColdboot();
  wedgeMgr->init();
  if (FLAGS_port_manager_mode) {
    portMgr->init();
  }

  // Refresh Transceivers
  auto tcvrIds = getMatchingTcvrIds(wedgeMgr, mediaType);
  for (auto tcvrId : tcvrIds) {
    suspender.dismiss();
    wedgeMgr->TransceiverManager::refreshTransceivers({tcvrId});
    suspender.rehire();
  }

  return tcvrIds.size();
}

std::size_t readOneByte(MediaInterfaceCode mediaType) {
  folly::BenchmarkSuspender suspender;
  // Making shared ptr so that we can use common helper function.
  std::shared_ptr<WedgeManager> wedgeMgr;
  std::unique_ptr<PortManager> portMgr;
  std::tie(wedgeMgr, portMgr) = setupForColdboot();
  wedgeMgr->init();
  if (FLAGS_port_manager_mode) {
    portMgr->init();
  }

  // Read Transceivers
  auto tcvrIds = getMatchingTcvrIds(wedgeMgr, mediaType);
  for (auto tcvrId : tcvrIds) {
    std::map<int32_t, ReadResponse> response;
    std::unique_ptr<ReadRequest> request(new ReadRequest);
    TransceiverIOParameters param;

    request->ids() = {static_cast<int>(tcvrId)};
    param.offset() = 0;
    param.length() = 1;
    request->parameter() = param;

    suspender.dismiss();
    wedgeMgr->readTransceiverRegister(response, std::move(request));
    suspender.rehire();
  }

  return tcvrIds.size();
}

std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
setupForColdboot() {
  // First use QsfpConfig to init default command line arguments
  initFlagDefaultsFromQsfpConfig();
  // Once we setup for cold boot, WedgeManager will always reload xphy firmware
  // when initExternalPhyMap() is called
  createDir(FLAGS_qsfp_service_volatile_dir);
  auto fd = createFile(TransceiverManager::forceColdBootFileName());
  close(fd);

  gflags::SetCommandLineOptionWithMode(
      "force_reload_gearbox_fw", "1", gflags::SET_FLAGS_DEFAULT);

  return createQsfpManagers();
}

std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
setupForWarmboot() {
  // First use QsfpConfig to init default command line arguments
  initFlagDefaultsFromQsfpConfig();
  // Use cold boot to force download xphy
  auto [wedgeMgr, portMgr] = setupForColdboot();
  if (!FLAGS_port_manager_mode) {
    wedgeMgr->initExternalPhyMap();
  } else {
    portMgr->initExternalPhyMap();
  }

  // Use gracefulExit so the next initExternalPhyMap() will be using warm boot
  if (FLAGS_port_manager_mode) {
    portMgr->gracefulExit();
  }
  wedgeMgr->gracefulExit();

  return createQsfpManagers();
}

} // namespace facebook::fboss
