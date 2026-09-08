/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/StatsPublisherHelper.h"
#include <fb303/ThreadCachedServiceData.h>
#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace {
// Helper to return the -7 from a BER of 1.5e-7
int getBerLog(double ber) {
  if (ber != 0) {
    return std::floor(std::log10(ber));
  }
  return -32;
}

// Helper to convert volt to milli volt
int vToMv(double v) {
  return v * 1000;
}

// Helper to convert milli watt to micro watt
int mWToUw(double mw) {
  return mw * 1000;
}

int scaledSnr(double snr) {
  return snr * 100;
}

int scaledMpi(double mpi) {
  return mpi * 100;
}
} // namespace

namespace facebook {
namespace fboss {

void StatsPublisherHelper::updateFb303KeysForSensors(
    const TransceiverInfo& info,
    const std::string& prefix) {
  if (auto sensor = info.tcvrStats()->sensor()) {
    tcData().setCounter(
        folly::to<std::string>(prefix, "temp"), *sensor->temp()->value());
    tcData().setCounter(
        folly::to<std::string>(prefix, "vcc.mv"),
        vToMv(*sensor->vcc()->value()));
  }
}

void StatsPublisherHelper::updateFb303KeysForChannels(
    const TransceiverInfo& info) {
  // Host side info
  for (const auto& portToLanesIt : *info.tcvrStats()->portNameToHostLanes()) {
    auto prefix = folly::to<std::string>(
        StatsPublisherHelper::kInterfacePrefix, portToLanesIt.first, ".");
    for (const auto& channelId : portToLanesIt.second) {
      if (channelId >= info.tcvrStats()->channels()->size()) {
        XLOG(ERR) << "Transceiver "
                  << folly::copy(info.tcvrState()->port().value())
                  << ":Host Stats not present for channel " << channelId;
        continue;
      }
      auto& channel = info.tcvrStats()->channels()->at(channelId);
      if (channel.sensors()->txSnr().has_value()) {
        tcData().setCounter(
            folly::to<std::string>(
                prefix, "txSnr.", "channel", *channel.channel()),
            scaledSnr(*channel.sensors()->txSnr()->value()));
      }
    }
  }
  // Media side info
  for (const auto& portToLanesIt : *info.tcvrStats()->portNameToMediaLanes()) {
    auto prefix = folly::to<std::string>(
        StatsPublisherHelper::kInterfacePrefix, portToLanesIt.first, ".");
    for (const auto& channelId : portToLanesIt.second) {
      if (channelId >= info.tcvrStats()->channels()->size()) {
        XLOG(ERR) << "Transceiver "
                  << folly::copy(info.tcvrState()->port().value())
                  << ":Media Stats not present for channel " << channelId;
        continue;
      }
      auto& channel = info.tcvrStats()->channels()->at(channelId);
      tcData().setCounter(
          folly::to<std::string>(
              prefix, "txBias.", "channel", *channel.channel()),
          *channel.sensors()->txBias()->value());
      tcData().setCounter(
          folly::to<std::string>(
              prefix, "txPwr.uW.", "channel", *channel.channel()),
          mWToUw(*channel.sensors()->txPwr()->value()));
      tcData().setCounter(
          folly::to<std::string>(
              prefix, "rxPwr.uW.", "channel", *channel.channel()),
          mWToUw(*channel.sensors()->rxPwr()->value()));
      if (channel.sensors()->txSnr().has_value()) {
        tcData().setCounter(
            folly::to<std::string>(
                prefix, "txSnr.", "channel", *channel.channel()),
            scaledSnr(*channel.sensors()->txSnr()->value()));
      }
      if (channel.sensors()->rxSnr().has_value()) {
        tcData().setCounter(
            folly::to<std::string>(
                prefix, "rxSnr.", "channel", *channel.channel()),
            scaledSnr(*channel.sensors()->rxSnr()->value()));
      }
    }
  }
}

void StatsPublisherHelper::updateFb303KeysForVdmCounters(
    const TransceiverInfo& info) {
  if (auto vdmStatsForOds = info.tcvrStats()->vdmPerfMonitorStatsForOds()) {
    for (const auto& sideStats : *vdmStatsForOds->mediaPortVdmStats()) {
      auto prefix = folly::to<std::string>(
          StatsPublisherHelper::kInterfacePrefix, sideStats.first, ".");
      tcData().setCounter(
          prefix + "preFecBerMediaMaxLog",
          getBerLog(sideStats.second.datapathBERMax().value()));
      tcData().setCounter(
          prefix + "errFrameMediaMax",
          sideStats.second.datapathErroredFramesMax().value());
      if (auto fecTail = sideStats.second.fecTailMax()) {
        tcData().setCounter(prefix + "fecTailMediaMax", *fecTail);
      }
      // TODO: Add SNR
    }
    for (const auto& sideStats : *vdmStatsForOds->hostPortVdmStats()) {
      auto prefix = folly::to<std::string>(
          StatsPublisherHelper::kInterfacePrefix, sideStats.first, ".");
      tcData().setCounter(
          prefix + "preFecBerHostMaxLog",
          getBerLog(sideStats.second.datapathBERMax().value()));
      tcData().setCounter(
          prefix + "errFrameHostMax",
          sideStats.second.datapathErroredFramesMax().value());
      if (auto fecTail = sideStats.second.fecTailMax()) {
        tcData().setCounter(prefix + "fecTailHostMax", *fecTail);
      }
      // TODO: Add SNR
    }
  }
  if (auto vdmStatsForOds = info.tcvrStats()->vdmPerfMonitorStats()) {
    uint8_t globalMpiAlarmFlag = 0;
    uint8_t globalMpiWarningFlag = 0;

    for (const auto& sideStats : *vdmStatsForOds->mediaPortVdmStats()) {
      auto prefix = folly::to<std::string>(
          StatsPublisherHelper::kInterfacePrefix, sideStats.first, ".");
      for (const auto& [lane, mpi] : sideStats.second.lanePam4MPI().value()) {
        tcData().setCounter(
            folly::to<std::string>(prefix, "mpi.", "channel", lane),
            scaledMpi(mpi));
      }

      // Publish MPI alarm/warning flags if available
      for (const auto& [lane, flags] :
           sideStats.second.lanePam4MPIFlags().value()) {
        // Publish high alarm flag (1 if alarm is active, 0 otherwise)
        tcData().setCounter(
            folly::to<std::string>(prefix, "mpi.alarm.high.", "channel", lane),
            *flags.alarm()->high() ? 1 : 0);

        // Publish high warning flag (1 if warning is active, 0 otherwise)
        tcData().setCounter(
            folly::to<std::string>(
                prefix, "mpi.warning.high.", "channel", lane),
            *flags.warn()->high() ? 1 : 0);

        if (*flags.alarm()->high()) {
          globalMpiAlarmFlag = globalMpiAlarmFlag | (1 << lane);
        }

        if (*flags.warn()->high()) {
          globalMpiWarningFlag = globalMpiWarningFlag | (1 << lane);
        }
      }
    }
    std::string transceiverName = *info.tcvrStats()->tcvrName();

    if (!transceiverName.empty()) {
      auto globalPrefix = folly::to<std::string>(
          StatsPublisherHelper::kInterfacePrefix, transceiverName, ".");

      tcData().setCounter(
          folly::to<std::string>(globalPrefix, "mpi.alarm.high.global"),
          globalMpiAlarmFlag);
      tcData().setCounter(
          folly::to<std::string>(globalPrefix, "mpi.warning.high.global"),
          globalMpiWarningFlag);
    }
  }
}

void StatsPublisherHelper::publishFb303Counters(
    const std::map<int32_t, TransceiverInfo>& infoMap,
    uint32_t stats_publish_interval,
    const TransceiverManager* transceiverManager) {
  int tcvrsWithErrorState = 0;
  for (const auto& kv : infoMap) {
    const TransceiverInfo& info = kv.second;
    auto portName = transceiverManager->getPortName(
        TransceiverID(*info.tcvrState()->port()));

    if (!(*info.tcvrState()->present()) || portName.empty()) {
      continue;
    }

    if (!info.tcvrState()->errorStates()->empty()) {
      tcvrsWithErrorState++;
    }

    auto interfacePrefix = folly::to<std::string>(
        StatsPublisherHelper::kInterfacePrefix, portName, ".");
    updateFb303KeysForSensors(info, interfacePrefix);
    updateFb303KeysForChannels(info);
  }

  tcData().setCounter(
      StatsPublisherHelper::kTcvrsWithErrors, tcvrsWithErrorState);

  for (const auto& kv : infoMap) {
    const TransceiverInfo& info = kv.second;
    auto portName = transceiverManager->getPortName(
        TransceiverID(*info.tcvrState()->port()));

    /* find the port name and use that to construct the prefix. */
    if (!portName.empty()) {
      updateFb303KeysForVdmCounters(info);
    } else {
      XLOG(ERR) << "Failed to find name for port " << *info.tcvrState()->port()
                << ".";
    }
  }
}

/*
 * triggerVdmStatsCapture
 *
 * Triggers the VDM data capture in all the modules after the stats publisher
 * has finished reporting to ODS. This will start a new cycle of VDM data
 * collection in modules.
 */
void StatsPublisherHelper::triggerVdmStatsCapture(
    std::map<int32_t, TransceiverInfo>& infoMap,
    TransceiverManager* transceiverManager) {
  std::vector<int32_t> portIdList;

  for (const auto& kv : infoMap) {
    const TransceiverInfo& info = kv.second;
    auto portId = info.tcvrState()->port().value();
    portIdList.push_back(portId);
  }
  transceiverManager->triggerVdmStatsCapture(portIdList);
}

} // namespace fboss
} // namespace facebook
