/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

class StatsPublisherHelper {
 public:
  static constexpr auto kInterfacePrefix = "qsfp.interface.";

  static constexpr auto opticsRemediationCounterName = "opticsRemediationCount";

  static constexpr auto kTcvrsWithErrors = "qsfp.transceivers_in_errored_state";

  void publishFb303Counters(
      const std::map<int32_t, TransceiverInfo>& infoMap,
      uint32_t stats_publish_interval,
      const TransceiverManager* transceiverManager);
  void triggerVdmStatsCapture(
      std::map<int32_t, TransceiverInfo>& infoMap,
      TransceiverManager* transceiverManager);

 private:
  void updateFb303KeysForVdmCounters(const TransceiverInfo& info);
  void updateFb303KeysForChannels(const TransceiverInfo& info);
  void updateFb303KeysForSensors(
      const TransceiverInfo& info,
      const std::string& prefix);

  std::unordered_map<int /* tcvrID */, int64_t /* timestamp */>
      lastVdmStatsUpdateTime;
};

} // namespace fboss
} // namespace facebook
