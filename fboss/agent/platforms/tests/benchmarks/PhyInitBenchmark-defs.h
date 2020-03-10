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
#include <folly/logging/xlog.h>

#include <fboss/agent/Platform.h>
#include <fboss/agent/platforms/wedge/WedgePlatform.h>
#include <fboss/agent/platforms/wedge/WedgePlatformInit.h>

DECLARE_bool(force_reload_gearbox_fw);

namespace facebook::fboss {

template <typename PlatformT, typename FpgaT>
std::unique_ptr<PlatformT> createPhyInitPlatform() {
  auto wedge = createWedgePlatform();
  auto plat = dynamic_cast<PlatformT*>(wedge.release());
  // initialize hw for fpga
  FpgaT::getInstance()->initHW();
  plat->init(createEmptyAgentConfig(), 0);

  return std::unique_ptr<PlatformT>(plat);
}

template <typename PlatformT, typename FpgaT>
void PhyInitAllForce() {
  folly::BenchmarkSuspender suspender;
  auto plat = createPhyInitPlatform<PlatformT, FpgaT>();

  XLOG(INFO) << "Force force_reload_gearbox_fw=true";
  FLAGS_force_reload_gearbox_fw = true;

  suspender.dismiss();
  plat->initAllPims(false /* warmBoot */);
}

template <typename PlatformT, typename FpgaT>
void PhyInitAllAuto() {
  folly::BenchmarkSuspender suspender;
  auto plat = createPhyInitPlatform<PlatformT, FpgaT>();

  XLOG(INFO) << "Force force_reload_gearbox_fw=true";
  FLAGS_force_reload_gearbox_fw = true;
  plat->initAllPims(false /* warmBoot */);

  XLOG(INFO) << "Force force_reload_gearbox_fw=false";
  FLAGS_force_reload_gearbox_fw = false;

  suspender.dismiss();
  plat->initAllPims(false /* warmBoot */);
}

} // namespace facebook::fboss
