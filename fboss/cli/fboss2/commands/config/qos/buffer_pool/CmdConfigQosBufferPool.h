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

#include <string>
#include <utility>
#include <vector>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

// Custom type for buffer pool configuration
// Parses: <name> <attr> <value> [<attr> <value> ...]
// where attr is one of: shared-bytes, headroom-bytes, reserved-bytes
class BufferPoolConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BufferPoolConfig(std::vector<std::string> v);

  const std::string& getName() const {
    return name_;
  }

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_BUFFER_POOL_NAME;

 private:
  std::string name_;
  std::vector<std::pair<std::string, std::string>> attributes_;
};

struct CmdConfigQosBufferPoolTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_BUFFER_POOL_NAME;
  using ObjectArgType = BufferPoolConfig;
  using RetType = std::string;
};

class CmdConfigQosBufferPool
    : public CmdHandler<CmdConfigQosBufferPool, CmdConfigQosBufferPoolTraits> {
 public:
  using ObjectArgType = CmdConfigQosBufferPoolTraits::ObjectArgType;
  using RetType = CmdConfigQosBufferPoolTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
