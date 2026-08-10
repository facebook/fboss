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
#include "fboss/cli/fboss2/commands/delete/qos/policy/CmdDeleteQosPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * Which QosMap a delete targets. Only the two map types the config command
 * writes as individually addressable entries are removable today; the
 * remaining types CmdConfigQosPolicyMap supports are rejected by name.
 */
enum class DeleteQosMapType {
  DSCP, // QosMap.dscpMaps
  TC_TO_QUEUE, // QosMap.trafficClassToQueueId
};

/**
 * Parses the entry to remove:
 *
 *   dscp <dscp-value>      - drop a DSCP codepoint from the policy's dscpMaps
 *   tc-to-queue <tc>       - drop a traffic class from trafficClassToQueueId
 *
 * The value alone identifies the entry: a DSCP codepoint appears in at most
 * one DscpQosMap, and a traffic class is a map key.
 */
class DeleteQosMapEntry : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ DeleteQosMapEntry(std::vector<std::string> v);

  DeleteQosMapType getMapType() const {
    return mapType_;
  }

  int16_t getKey() const {
    return key_;
  }

 private:
  DeleteQosMapType mapType_{DeleteQosMapType::TC_TO_QUEUE};
  int16_t key_{0};
};

struct CmdDeleteQosPolicyMapTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteQosPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "map_entry",
           args,
           "<map-type> <value> where map-type is one of:\n"
           "  dscp <dscp-value>   remove a DSCP to traffic-class mapping\n"
           "  tc-to-queue <tc>    remove a traffic-class to queue mapping")
        ->required()
        ->expected(2);
  }
  using ObjectArgType = DeleteQosMapEntry;
  using RetType = std::string;
};

class CmdDeleteQosPolicyMap
    : public CmdHandler<CmdDeleteQosPolicyMap, CmdDeleteQosPolicyMapTraits> {
 public:
  using ObjectArgType = CmdDeleteQosPolicyMapTraits::ObjectArgType;
  using RetType = CmdDeleteQosPolicyMapTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const QosPolicyName& policyName,
      const ObjectArgType& entry);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
