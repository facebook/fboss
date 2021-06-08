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

namespace facebook::fboss {

WedgeManager* HwQsfpEnsemble::getWedgeManager() {
  return static_cast<WedgeManager*>(
      qsfpServiceHandler_->getTransceiverManager());
}
void HwQsfpEnsemble::init() {
  auto wedgeManager = createWedgeManager();
  std::tie(server_, qsfpServiceHandler_) =
      setupThriftServer(std::move(wedgeManager));
}

HwQsfpEnsemble::~HwQsfpEnsemble() {
  if (FLAGS_setup_thrift) {
    doServerLoop(server_, qsfpServiceHandler_);
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
} // namespace facebook::fboss
