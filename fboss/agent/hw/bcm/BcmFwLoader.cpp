/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmFwLoader.h"

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <fstream>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

DEFINE_string(qcm_fw_path, "", "Location of qcm firmware");
DEFINE_bool(load_qcm_fw, false, "Enable load of qcm fimrware");

namespace {
static void loadQcmFw(
    const facebook::fboss::BcmSwitch* sw,
    const int ucontrollerId) {
  if (FLAGS_qcm_fw_path.empty()) {
    return;
  }

  std::ifstream fwFile(FLAGS_qcm_fw_path);
  if (!fwFile.good()) {
    throw facebook::fboss::FbossError(
        "Micro controller firmware file not found: ", FLAGS_qcm_fw_path);
  }

  // Based on bcm csp CS9941424, inorder for qcm fw to warm boot
  // we need to gracefully shutdown communication with ucontroller(uc) prior
  // to loading the new fw and after load, explicitly initializing
  // the communication with uc (using 'msmsg init', 'mcmsg ')
  // Stop messaging with uc
  std::string cmd = folly::to<std::string>("mcsmsg stop ", ucontrollerId);
  sw->printDiagCmd(cmd.c_str());

  // load fw
  cmd = folly::to<std::string>(
      "mcsload ", ucontrollerId, " ", FLAGS_qcm_fw_path, " InitMCS=true");
  sw->printDiagCmd(cmd.c_str());

  // check status
  // This is mostly for debugging purposes and is not a requirement
  cmd = "mcsstatus";
  sw->printDiagCmd(cmd.c_str());

  cmd = "mcsmsg Init";
  sw->printDiagCmd(cmd.c_str());

  cmd = folly::to<std::string>("mcsmsg ", ucontrollerId);
  sw->printDiagCmd(cmd.c_str());
}

static const std::vector<facebook::fboss::BcmFirmware> fwImages = {
    {
        .fwType = facebook::fboss::FwType::QCM,
        .sdkSpecific = false,
        .core_id = 0,
        .addr = 0,
        .bcmFwLoadFunc_ = loadQcmFw,
    },
};
} // namespace

namespace facebook {
namespace fboss {

void BcmFwLoader::loadFirmwareImpl(BcmSwitch* sw, FwType fwType) {
  // Find fw
  auto fwIt = std::find_if(
      fwImages.begin(), fwImages.end(), [&fwType](const auto& f) -> bool {
        return f.fwType == fwType;
      });

  if (fwIt == fwImages.end()) {
    throw FbossError("Micro controller firmware not found, type : ", fwType);
  }

  const BcmFirmware& fw = *fwIt;
  fw.bcmFwLoadFunc_(sw, fw.core_id);
}

void BcmFwLoader::loadFirmware(BcmSwitch* sw, PlatformMode platformMode) {
  switch (platformMode) {
    case PlatformMode::WEDGE100:
      // QCM fw is supported on wedge100 only
      if (FLAGS_load_qcm_fw) {
        XLOG(INFO) << "Load the QCM firmware ..";
        loadFirmwareImpl(sw, FwType::QCM);
      }
      break;
    default:
      // no firmware needed for other chips
      break;
  }
  return;
}

} // namespace fboss
} // namespace facebook
