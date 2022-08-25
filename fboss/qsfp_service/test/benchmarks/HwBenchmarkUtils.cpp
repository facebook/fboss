// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

namespace facebook::fboss {

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
