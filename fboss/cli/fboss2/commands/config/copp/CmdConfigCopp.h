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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `config copp cpu-queue <id> [<sub-cmd> <value>]`.
//
// Accepted forms (validated by CoppCpuQueueArgs):
//   <id>                        -> ensure queue <id> exists
//   <id> name <string>          -> set name
//   <id> rate-limit kbps <max>  -> set max bandwidth in kbps
//   <id> rate-limit pps <max>   -> set max packet rate in pps
//
// The accepted integer range for <id> is 0..255 (CPU queue IDs are a small,
// platform-bounded set; the exact cap depends on the ASIC via
// SaiHostifManager::getMaxCpuQueues). Value parsing for <max> is deferred to
// applyCpuQueueConfig() so error messages can name the specific attribute.
class CoppCpuQueueArgs : public utils::BaseObjectArgType<std::string> {
 public:
  enum class Op {
    // No sub-command: just ensure the queue exists.
    NONE,
    // name <string>
    NAME,
    // rate-limit kbps <max>
    RATE_LIMIT_KBPS,
    // rate-limit pps <max>
    RATE_LIMIT_PPS,
  };

  /* implicit */ CoppCpuQueueArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  int16_t getQueueId() const {
    return queueId_;
  }

  Op getOp() const {
    return op_;
  }

  // Only meaningful when op == NAME.
  const std::string& getName() const {
    return name_;
  }

  // Only meaningful when op == RATE_LIMIT_KBPS or RATE_LIMIT_PPS.
  int32_t getRateMax() const {
    return rateMax_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_COPP_CPU_QUEUE_CONFIG;

 private:
  int16_t queueId_ = 0;
  Op op_ = Op::NONE;
  std::string name_;
  int32_t rateMax_ = 0;
};

// Argument for `config copp reason <reason-name> queue <id>`.
//
// <reason-name> is matched case-insensitively against the cfg::PacketRxReason
// enum (e.g. arp, ndp, bgp, lacp, lldp, dhcp, dhcpv6, bgpv6, ttl_1, ...).
// <id> is validated as a non-negative int16.
class CoppReasonArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ CoppReasonArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  cfg::PacketRxReason getReason() const {
    return reason_;
  }

  int16_t getQueueId() const {
    return queueId_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_COPP_REASON_CONFIG;

 private:
  cfg::PacketRxReason reason_{};
  int16_t queueId_ = 0;
};

// The `copp` parent node itself is not usable; it only exists to dispatch to
// cpu-queue and reason. The parent needs a handler (rather than being a pure
// branch node) so that addCommandBranch() increments depth before descending
// into the leaves — without that, cpu-queue and reason would both register
// their positional args at the same CmdArgsLists slot, and CLI11 parsing
// collides with siblings of `config` whose names happen to also be valid
// reason names (e.g. `arp`).
struct CmdConfigCoppTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdConfigCopp : public CmdHandler<CmdConfigCopp, CmdConfigCoppTraits> {
 public:
  using ObjectArgType = CmdConfigCoppTraits::ObjectArgType;
  using RetType = CmdConfigCoppTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use 'cpu-queue' or 'reason' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

struct CmdConfigCoppCpuQueueTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigCopp;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_COPP_CPU_QUEUE_CONFIG;
  using ObjectArgType = CoppCpuQueueArgs;
  using RetType = std::string;
};

class CmdConfigCoppCpuQueue
    : public CmdHandler<CmdConfigCoppCpuQueue, CmdConfigCoppCpuQueueTraits> {
 public:
  using ObjectArgType = CmdConfigCoppCpuQueueTraits::ObjectArgType;
  using RetType = CmdConfigCoppCpuQueueTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

struct CmdConfigCoppReasonTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigCopp;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_COPP_REASON_CONFIG;
  using ObjectArgType = CoppReasonArgs;
  using RetType = std::string;
};

class CmdConfigCoppReason
    : public CmdHandler<CmdConfigCoppReason, CmdConfigCoppReasonTraits> {
 public:
  using ObjectArgType = CmdConfigCoppReasonTraits::ObjectArgType;
  using RetType = CmdConfigCoppReasonTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
