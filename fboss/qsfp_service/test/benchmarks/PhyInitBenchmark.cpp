/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Benchmark.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

namespace facebook::fboss {

std::unique_ptr<WedgeManager> setupForColdboot() {
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
  // Use cold boot to force download xphy
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->initExternalPhyMap();
  // Use gracefulExit so the next initExternalPhyMap() will be using warm boot
  wedgeMgr->gracefulExit();

  return createWedgeManager();
}

BENCHMARK(PhyInitAllColdBoot) {
  folly::BenchmarkSuspender suspender;
  auto wedgeMgr = setupForColdboot();
  suspender.dismiss();

  wedgeMgr->initExternalPhyMap();
}

BENCHMARK(PhyInitAllWarmBoot) {
  folly::BenchmarkSuspender suspender;
  auto wedgeMgr = setupForWarmboot();
  suspender.dismiss();

  wedgeMgr->initExternalPhyMap();
}

} // namespace facebook::fboss
