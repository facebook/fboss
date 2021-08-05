/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/MultiPimPlatformMapping.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

DEFINE_bool(
    setup_thrift,
    false,
    "Setup a thrift handler. Primarily useful for inspecting HW state,"
    "say for debugging things via a shell");

DECLARE_string(qsfp_service_volatile_dir);

namespace {
auto constexpr kQsfpTestWarmnbootFile = "can_warm_boot";
}
namespace facebook::fboss {

WedgeManager* HwQsfpEnsemble::getWedgeManager() {
  return static_cast<WedgeManager*>(
      qsfpServiceHandler_->getTransceiverManager());
}
void HwQsfpEnsemble::init() {
  warmBoot_ = removeFile(folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kQsfpTestWarmnbootFile));
  auto wedgeManager = createWedgeManager(!warmBoot_);
  std::tie(server_, qsfpServiceHandler_) =
      setupThriftServer(std::move(wedgeManager));
}

HwQsfpEnsemble::~HwQsfpEnsemble() {
  if (FLAGS_setup_thrift) {
    doServerLoop(server_, qsfpServiceHandler_);
  }
}

bool HwQsfpEnsemble::hasWarmbootCapability() const {
  return getWedgeManager()->getPlatformMode() != PlatformMode::ELBERT;
}

void HwQsfpEnsemble::setupForWarmboot() const {
  if (hasWarmbootCapability()) {
    createFile(folly::to<std::string>(
        FLAGS_qsfp_service_volatile_dir, "/", kQsfpTestWarmnbootFile));
  }
}

PhyManager* HwQsfpEnsemble::getPhyManager() {
  return getWedgeManager()->getPhyManager();
}

const PlatformMapping* HwQsfpEnsemble::getPlatformMapping() const {
  return getWedgeManager()->getPlatformMapping();
}

phy::ExternalPhy* HwQsfpEnsemble::getExternalPhy(PortID port) {
  auto phyManager = getPhyManager();
  return phyManager->getExternalPhy(port);
}

bool HwQsfpEnsemble::isXphyPlatform() const {
  static const std::set<PlatformMode> xphyPlatforms = {
      PlatformMode::MINIPACK,
      PlatformMode::YAMP,
      PlatformMode::FUJI,
      PlatformMode::ELBERT,
      PlatformMode::CLOUDRIPPER};

  return xphyPlatforms.find(getWedgeManager()->getPlatformMode()) !=
      xphyPlatforms.end();
}

bool HwQsfpEnsemble::isSaiPlatform() const {
  static const std::set<PlatformMode> saiPlatforms = {
      PlatformMode::ELBERT, PlatformMode::CLOUDRIPPER};

  return saiPlatforms.find(getWedgeManager()->getPlatformMode()) !=
      saiPlatforms.end();
}
} // namespace facebook::fboss
