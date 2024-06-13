// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

std::size_t refreshTcvrs(MediaInterfaceCode mediaType) {
  folly::BenchmarkSuspender suspender;
  std::size_t iters = 0;
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->init();

  for (int i = 0; i < wedgeMgr->getNumQsfpModules(); i++) {
    TransceiverID id(i);
    auto interface =
        wedgeMgr->getTransceiverInfo(id).tcvrState()->moduleMediaInterface();

    if (interface.has_value() && interface.value() == mediaType) {
      std::unordered_set<TransceiverID> tcvr{id};

      suspender.dismiss();
      wedgeMgr->TransceiverManager::refreshTransceivers(tcvr);
      suspender.rehire();
      iters++;
    }
  }

  return iters;
}

std::size_t readOneByte(MediaInterfaceCode mediaType) {
  folly::BenchmarkSuspender suspender;
  std::size_t iters = 0;
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->init();

  for (int i = 0; i < wedgeMgr->getNumQsfpModules(); i++) {
    TransceiverID id(i);
    auto interface =
        wedgeMgr->getTransceiverInfo(id).tcvrState()->moduleMediaInterface();

    if (interface.has_value() && interface.value() == mediaType) {
      std::unordered_set<TransceiverID> tcvr{id};
      std::map<int32_t, ReadResponse> response;
      std::unique_ptr<ReadRequest> request(new ReadRequest);
      TransceiverIOParameters param;

      request->ids() = {i};
      param.offset() = 0;
      param.length() = 1;
      request->parameter() = param;
      suspender.dismiss();
      wedgeMgr->readTransceiverRegister(response, std::move(request));
      suspender.rehire();
      iters++;
    }
  }

  return iters;
}

std::unique_ptr<WedgeManager> setupForColdboot() {
  // First use QsfpConfig to init default command line arguments
  initFlagDefaultsFromQsfpConfig();
  // Once we setup for cold boot, WedgeManager will always reload xphy firmware
  // when initExternalPhyMap() is called
  createDir(FLAGS_qsfp_service_volatile_dir);
  auto fd = createFile(TransceiverManager::forceColdBootFileName());
  close(fd);

  gflags::SetCommandLineOptionWithMode(
      "force_reload_gearbox_fw", "1", gflags::SET_FLAGS_DEFAULT);

  return createWedgeManager();
}

std::unique_ptr<WedgeManager> setupForWarmboot() {
  // First use QsfpConfig to init default command line arguments
  initFlagDefaultsFromQsfpConfig();
  // Use cold boot to force download xphy
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->initExternalPhyMap();
  // Use gracefulExit so the next initExternalPhyMap() will be using warm boot
  wedgeMgr->gracefulExit();

  return createWedgeManager();
}

} // namespace facebook::fboss
