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

#include "Thrifty.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace facebook::fboss {

using BufferPoolCfgPtr = std::shared_ptr<BufferPoolCfg>;

USE_THRIFT_COW(PortPgConfig);
RESOLVE_STRUCT_MEMBER(
    PortPgConfig,
    switch_state_tags::bufferPoolConfig,
    BufferPoolCfg);

/*
 * PortPgConfig defines the behaviour of the per port queues
 */
class PortPgConfig
    : public ThriftStructNode<PortPgConfig, state::PortPgFields> {
 public:
  using BaseT = ThriftStructNode<PortPgConfig, state::PortPgFields>;
  explicit PortPgConfig(uint8_t id) {
    set<switch_state_tags::id>(id);
  }

  // std::string toString() const;

  uint8_t getID() const {
    return safe_cref<switch_state_tags::id>()->toThrift();
  }

  int getMinLimitBytes() const {
    return safe_cref<switch_state_tags::minLimitBytes>()->toThrift();
  }

  void setMinLimitBytes(int minLimitBytes) {
    ref<switch_state_tags::minLimitBytes>() = minLimitBytes;
  }

  std::optional<cfg::MMUScalingFactor> getScalingFactor() const {
    if (auto scalingFactor = safe_cref<switch_state_tags::scalingFactor>()) {
      cfg::MMUScalingFactor cfgMMUScalingFactor{};
      apache::thrift::util::tryParseEnum(
          scalingFactor->toThrift(), &cfgMMUScalingFactor);
      return cfgMMUScalingFactor;
    }
    return std::nullopt;
  }

  void setScalingFactor(cfg::MMUScalingFactor scalingFactor) {
    set<switch_state_tags::scalingFactor>(
        apache::thrift::util::enumName(scalingFactor));
  }

  std::optional<std::string> getName() const {
    if (auto name = safe_cref<switch_state_tags::name>()) {
      return name->toThrift();
    }
    return std::nullopt;
  }

  void setName(const std::string& name) {
    set<switch_state_tags::name>(name);
  }

  std::optional<int> getHeadroomLimitBytes() const {
    if (auto headroomLimitBytes =
            safe_cref<switch_state_tags::headroomLimitBytes>()) {
      return headroomLimitBytes->toThrift();
    }
    return std::nullopt;
  }

  void setHeadroomLimitBytes(int headroomLimitBytes) {
    set<switch_state_tags::headroomLimitBytes>(headroomLimitBytes);
  }

  std::optional<int> getResumeOffsetBytes() const {
    if (auto resumeOffsetBytes =
            safe_cref<switch_state_tags::resumeOffsetBytes>()) {
      return resumeOffsetBytes->toThrift();
    }
    return std::nullopt;
  }

  void setResumeOffsetBytes(int resumeOffsetBytes) {
    set<switch_state_tags::resumeOffsetBytes>(resumeOffsetBytes);
  }

  std::string getBufferPoolName() const {
    return safe_cref<switch_state_tags::bufferPoolName>()->toThrift();
  }

  void setBufferPoolName(const std::string& name) {
    ref<switch_state_tags::bufferPoolName>() = name;
  }

  std::optional<BufferPoolCfgPtr> getBufferPoolConfig() const {
    if (auto bufferPoolConfigPtr =
            safe_cref<switch_state_tags::bufferPoolConfig>()) {
      return bufferPoolConfigPtr;
    }
    return std::nullopt;
  }

  void setBufferPoolConfig(BufferPoolCfgPtr bufferPoolConfigPtr) {
    ref<switch_state_tags::bufferPoolConfig>() = bufferPoolConfigPtr;
  }

  static state::PortPgFields makeThrift(
      uint8_t id,
      std::optional<cfg::MMUScalingFactor> scalingFactor,
      const std::optional<std::string>& name,
      int minLimitBytes,
      std::optional<int> headroomLimitBytes,
      std::optional<int> resumeOffsetBytes,
      const std::string& bufferPoolName,
      const std::optional<BufferPoolFields>& bufferPoolConfig = std::nullopt) {
    state::PortPgFields obj{};
    obj.id() = id;
    if (scalingFactor) {
      obj.scalingFactor() = apache::thrift::util::enumName(*scalingFactor);
    }
    if (name) {
      obj.name() = *name;
    }
    obj.minLimitBytes() = minLimitBytes;
    if (headroomLimitBytes) {
      obj.headroomLimitBytes() = *headroomLimitBytes;
    }
    if (resumeOffsetBytes) {
      obj.resumeOffsetBytes() = *resumeOffsetBytes;
    }
    obj.bufferPoolName() = bufferPoolName;
    if (bufferPoolConfig) {
      obj.bufferPoolConfig() = *bufferPoolConfig;
    }
    return obj;
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using PortPgConfigs = std::vector<std::shared_ptr<PortPgConfig>>;
} // namespace facebook::fboss
