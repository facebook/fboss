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

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <fstream>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

// QCM firmware has suffix of <BCM56960>_<version_number>.
// <version_number> has been bumped to 1 since BCM provided new release of
// the firmware.
DEFINE_string(
    qcm_fw_path,
    "/var/facebook/fboss/BCM56960_2_qcm.srec",
    "Location of qcm firmware");
DEFINE_bool(load_qcm_fw, false, "Enable load of qcm fimrware");
DEFINE_bool(
    ignore_qcm_fw_file_not_found,
    true,
    "Ignore if qcm fw file is not found");

namespace {
static void loadQcmFw(
    const facebook::fboss::BcmSwitch* sw,
    const int ucontrollerId,
    const std::string& fwFileName) {
  // Based on bcm csp CS9941424, inorder for qcm fw to warm boot
  // we need to gracefully shutdown communication with ucontroller(uc) prior
  // to loading the new fw and after load, explicitly initializing
  // the communication with uc (using 'msmsg init', 'mcmsg ')
  // Stop messaging with uc
  std::string cmd = folly::to<std::string>("mcsmsg stop ", ucontrollerId);
  sw->printDiagCmd(cmd.c_str());

  // load fw
  cmd = folly::to<std::string>(
      "mcsload ", ucontrollerId, " ", fwFileName, " InitMCS=true");
  sw->printDiagCmd(cmd.c_str());

  // check status
  // This is mostly for debugging purposes and is not a requirement
  cmd = "mcsstatus";
  sw->printDiagCmd(cmd.c_str());
}

static void stopQcmFw(
    const facebook::fboss::BcmSwitch* sw,
    const int ucontrollerId) {
  std::string cmd = folly::to<std::string>("mcsmsg stop ", ucontrollerId);
  sw->printDiagCmd(cmd.c_str());
}

static std::string loadQcmFileFile() {
  return FLAGS_qcm_fw_path;
}

// this func is invoked if we fail to load the relevant QCM file
// as its not found
static void fwQcmFileErrorHandler(const std::string& fwFileName) {
  facebook::fb303::fbData->incrementCounter("qcm_fw_not_found_failure", 1);
  const std::string errorStr = folly::to<std::string>(
      "QCM Micro controller firmware file not found: " + fwFileName);
  // since qcm fw file has sdk suffix in it, its possible that for some
  // combinations we don't have the correct fw file on the switch. We want to
  // not fail the bcm init simply because of it (as it has no bearing on non-qcm
  // functionality) Raise an alarm. WIll like to fail hard though when in test
  // mode
  if (FLAGS_ignore_qcm_fw_file_not_found) {
    XLOG(ERR) << errorStr << " . Ignore the error!";
    return;
  }
  throw facebook::fboss::FbossError(errorStr);
}

static const std::vector<facebook::fboss::BcmFirmware> fwImages = {
    {
        .fwType = facebook::fboss::FwType::QCM,
        .sdkSpecific = true,
        .core_id = 0,
        .addr = 0,
        .fwFileErrorHandler_ = fwQcmFileErrorHandler,
        .getFwFile_ = loadQcmFileFile,
        .bcmFwLoadFunc_ = loadQcmFw,
        .bcmFwStopFunc_ = stopQcmFw,
    },
};
} // namespace

namespace facebook {
namespace fboss {

std::string BcmFwLoader::getFwFile(const BcmFirmware& fw) {
  std::string fwFileName = fw.getFwFile_();
  if (fwFileName.empty()) {
    return {};
  }

  if (fw.sdkSpecific) {
#if (!defined(BCM_VER_MAJOR))
    XLOG(WARNING) << "Firmware OpenNSA version missing";
#else
    std::string sdkPostfix = folly::to<std::string>(
        "-", BCM_VER_MAJOR, ".", BCM_VER_MINOR, ".", BCM_VER_RELEASE);
    fwFileName.append(sdkPostfix);
#endif
  }

  // vaidate fw file exists
  std::ifstream fwFile(fwFileName);
  if (!fwFile.good()) {
    // fw file load error
    fw.fwFileErrorHandler_(fwFileName);
    return {};
  }
  return fwFileName;
}

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
  auto const fwFileName = getFwFile(fw);
  if (!fwFileName.empty()) {
    fw.bcmFwLoadFunc_(sw, fw.core_id, fwFileName);
  }
}

void BcmFwLoader::stopFirmwareImpl(BcmSwitch* sw, FwType fwType) {
  // Find fw
  auto fwIt = std::find_if(
      fwImages.begin(), fwImages.end(), [&fwType](const auto& f) -> bool {
        return f.fwType == fwType;
      });

  if (fwIt == fwImages.end()) {
    throw FbossError("Micro controller firmware not found, type : ", fwType);
  }
  const BcmFirmware& fw = *fwIt;
  fw.bcmFwStopFunc_(sw, fw.core_id);
}

void BcmFwLoader::loadFirmware(BcmSwitch* sw, HwAsic* hwAsic) {
  if (hwAsic && hwAsic->isSupported(HwAsic::Feature::QCM)) {
    // QCM fw is supported on Tomahawk only
    XLOG(INFO) << "Load the QCM firmware ..";
    loadFirmwareImpl(sw, FwType::QCM);
  }
  return;
}

void BcmFwLoader::stopFirmware(BcmSwitch* sw) {
  stopFirmwareImpl(sw, FwType::QCM);
}
} // namespace fboss
} // namespace facebook
