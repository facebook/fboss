// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"

#include <folly/FileUtil.h>
#include <filesystem>

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

void switchPreInitSequence(cfg::AsicType asicType) {
  if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
    return;
  }
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  // Generate SOC file with register/table settings to enable PFC based on OBM
  // usage at per port level and to enable FC between EGQ / FDR-FDA blocks,
  // which will help avoid drops at ingress and egress respectively with higher
  // GPU NCCL collective runs.
  // TODO: Remove these once the patch for CS00012371269 and CS00012370885 are
  // available.
  const std::string kSaiPostInitCmdFileContent{
      "w CGM_VSQE_RJCT_PRMS 0 128 -1 -1 -1 -1\n"
      "mod CGM_PB_VSQ_RJCT_MASK 0 16 VSQ_RJCT_MASK=0x0\n"
      "mod CGM_VSQF_FC_PRMS 0 16 WORDS_MIN_TH=60000 WORDS_MAX_TH=60000 WORDS_OFFSET=20000 SRAM_BUFFERS_MAX_TH=2048 SRAM_BUFFERS_MIN_TH=256 SRAM_BUFFERS_OFFSET=128\n"
      "m ECGM_CONGESTION_MANAGEMENT_GLOBAL_THRESHOLDS TOTAL_DB_STOP_TH=29000\n"};
  const std::string kSaiPostInitCmdFilePath{"/tmp/sai_postinit_cmd_file.soc"};
  std::filesystem::create_directories(
      std::filesystem::path(kSaiPostInitCmdFilePath).parent_path().u8string());
  folly::writeFile(kSaiPostInitCmdFileContent, kSaiPostInitCmdFilePath.c_str());
#endif
}
} // namespace facebook::fboss
