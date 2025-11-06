// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

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
      case SAI_SWITCH_STAT_DEVICE_DRAM_BLOCK_TOTAL_TIME:
        hwSwitchDramStats.dramBlockedTimeNsec() = value;
        break;
#endif
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_SWITCH_STAT_DRAM_QUARANTINE_BUFFER_STATUS:
        hwSwitchDramStats.dramQuarantinedBufferCount() = value;
        break;
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
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0) && !defined(BRCM_SAI_SDK_DNX_GTE_14_0)
        // TODO (nivinl): Stats ID not yet available in 14.x!
      case SAI_SWITCH_STAT_EXTENSION_FABRIC_INTER_CELL_JITTER_MAX_IN_NSEC: {
        hwSwitchWatermarkStats.fabricInterCellJitterWatermarkUsec() = value;
        break;
      }
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
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_SWITCH_STAT_DDP_PACKET_ERROR:
        dropStats.dramDataPathPacketError() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void fillHwSwitchSaiExtensionDropStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& dropStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_SWITCH_STAT_EGR_RX_PACKET_ERROR:
        dropStats.packetIntegrityDrops() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

void fillHwSwitchPipelineStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    int idx,
    HwSwitchPipelineStats& hwSwitchPipelineStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_SWITCH_PIPELINE_STAT_RX_CELLS:
        hwSwitchPipelineStats.rxCells()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_TX_CELLS:
        hwSwitchPipelineStats.txCells()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_RX_BYTES:
        hwSwitchPipelineStats.rxBytes()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_TX_BYTES:
        hwSwitchPipelineStats.txBytes()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_TX_WATERMARK_LEVELS:
        hwSwitchPipelineStats.txWatermarkLevels()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_CURR_OCCUPANCY_BYTES:
        hwSwitchPipelineStats.curOccupancyBytes()[idx] = value;
        break;
      case SAI_SWITCH_PIPELINE_STAT_GLOBAL_DROP:
        hwSwitchPipelineStats.globalDrops()[idx] = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

} // namespace facebook::fboss
