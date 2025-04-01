// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"

#include <folly/File.h>
#include <folly/FileUtil.h>

extern "C" {
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiswitchextensions.h>
#else
#include <saiswitchextensions.h>
#endif
}

namespace facebook::fboss {

void fillHwSwitchDramStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDramStats& hwSwitchDramStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX)
      case SAI_SWITCH_STAT_DEVICE_DRAM_ENQUEUED_BYTES:
        hwSwitchDramStats.dramEnqueuedBytes() = value;
        break;
      case SAI_SWITCH_STAT_DEVICE_DRAM_DEQUEUED_BYTES:
        hwSwitchDramStats.dramDequeuedBytes() = value;
        break;
#if defined(BRCM_SAI_SDK_GTE_11_0)
      case SAI_SWITCH_STAT_DEVICE_DRAM_BLOCK_TOTAL_TIME:
        hwSwitchDramStats.dramBlockedTimeNsec() = value;
        break;
#endif
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void fillHwSwitchWatermarkStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchWatermarkStats& hwSwitchWatermarkStats) {
  hwSwitchWatermarkStats = {};
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      case SAI_SWITCH_STAT_DEVICE_RCI_WATERMARK_BYTES:
        hwSwitchWatermarkStats.fdrRciWatermarkBytes() = value;
        break;
      case SAI_SWITCH_STAT_DEVICE_CORE_RCI_WATERMARK_BYTES:
        hwSwitchWatermarkStats.coreRciWatermarkBytes() = value;
        break;
      case SAI_SWITCH_STAT_HIGHEST_QUEUE_CONGESTION_LEVEL:
        hwSwitchWatermarkStats.dtlQueueWatermarkBytes() = value;
        break;
      case SAI_SWITCH_STAT_DEVICE_EGRESS_DB_WM:
        hwSwitchWatermarkStats.egressCoreBufferWatermarkBytes() = value;
        break;
      case SAI_SWITCH_STAT_ING_MIN_SRAM_BUFFER_BYTES:
        hwSwitchWatermarkStats.sramMinBufferWatermarkBytes() = value;
        break;
      case SAI_SWITCH_STAT_FDR_RX_QUEUE_WM_LEVEL:
        hwSwitchWatermarkStats.fdrFifoWatermarkBytes() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void fillHwSwitchCreditStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchCreditStats& hwSwitchCreditStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      case SAI_SWITCH_STAT_DEVICE_DELETED_CREDIT_COUNTER:
        hwSwitchCreditStats.deletedCreditBytes() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void fillHwSwitchErrorStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& dropStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      case SAI_SWITCH_STAT_EGRESS_FABRIC_CELL_ERROR:
        dropStats.rqpFabricCellCorruptionDrops() = value;
        break;
      case SAI_SWITCH_STAT_EGRESS_NON_FABRIC_CELL_ERROR:
        dropStats.rqpNonFabricCellCorruptionDrops() = value;
        break;
      case SAI_SWITCH_STAT_EGRESS_CUP_NON_FABRIC_CELL_ERROR:
        dropStats.rqpNonFabricCellMissingDrops() = value;
        break;
      case SAI_SWITCH_STAT_EGRESS_PARITY_CELL_ERROR:
        dropStats.rqpParityErrorDrops() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void switchPreInitSequence(HwAsic* asic) {
  // Below sequence is to avoid traffic drops on FE13 in 2-stage
  // on drain/undrain as reported in S503928.
  if ((asic->getSwitchType() != cfg::SwitchType::FABRIC) ||
      (asic->getFabricNodeRole() != HwAsic::FabricNodeRole::DUAL_STAGE_L1)) {
    return;
  }
  const std::string kSaiPostInitCmdFileContent{
      "s RTP_GRACEFUL_POWER_DOWN_CONFIGURATION 0\n"};
  const std::string kSaiPostInitCmdFilePath{"/tmp/sai_postinit_cmd_file.soc"};
  // Create a single file irrespective of the number of processes!
  try {
    folly::File file(kSaiPostInitCmdFilePath, O_WRONLY | O_CREAT | O_EXCL);
    if (folly::writeFull(
            file.fd(),
            kSaiPostInitCmdFileContent.c_str(),
            kSaiPostInitCmdFileContent.size()) == -1) {
      XLOG(ERR) << "Error writing to " << kSaiPostInitCmdFilePath
                << ", errno: " << errno;
      return;
    }
    // Make sure that the file is in the disk as its needed post SAI/SDK init
    fsync(file.fd());
    XLOG(DBG2) << "Created soc file " << kSaiPostInitCmdFilePath;
  } catch (const std::system_error& e) {
    if (e.code().value() == EEXIST) {
      XLOG(DBG2) << "File " << kSaiPostInitCmdFilePath << " already exists!";
    } else {
      XLOG(ERR) << "Error creating file: " << kSaiPostInitCmdFilePath << ", "
                << e.what();
    }
  }
}

} // namespace facebook::fboss
