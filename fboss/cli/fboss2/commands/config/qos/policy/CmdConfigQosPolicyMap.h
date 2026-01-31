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

#include <cstdint>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * Enum representing which QosMap to modify.
 */
enum class QosMapType {
  TC_TO_QUEUE, // trafficClassToQueueId
  PFC_PRI_TO_QUEUE, // pfcPriorityToQueueId
  TC_TO_PG, // trafficClassToPgId
  PFC_PRI_TO_PG // pfcPriorityToPgId
};

/**
 * Custom type for QoS map entry configuration.
 *
 * Parses command line arguments in the format:
 *   <map-type> <key> <value>
 *
 * For example:
 *   tc-to-queue 0 0
 *   pfc-pri-to-queue 3 3
 */
class QosMapConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QosMapConfig(std::vector<std::string> v);

  QosMapType getMapType() const {
    return mapType_;
  }

  int16_t getKey() const {
    return key_;
  }

  int16_t getValue() const {
    return value_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QOS_MAP_ENTRY;

 private:
  QosMapType mapType_{QosMapType::TC_TO_QUEUE};
  int16_t key_{0};
  int16_t value_{0};
};

struct CmdConfigQosPolicyMapTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQosPolicy;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_QOS_MAP_ENTRY;
  using ObjectArgType = QosMapConfig;
  using RetType = std::string;
};

class CmdConfigQosPolicyMap
    : public CmdHandler<CmdConfigQosPolicyMap, CmdConfigQosPolicyMapTraits> {
 public:
  using ObjectArgType = CmdConfigQosPolicyMapTraits::ObjectArgType;
  using RetType = CmdConfigQosPolicyMapTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const QosPolicyName& policyName,
      const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
