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

struct PortPgFields : public ThriftyFields<PortPgFields, state::PortPgFields> {
  PortPgFields() {}
  PortPgFields(
      uint8_t _id,
      std::optional<cfg::MMUScalingFactor> _scalingFactor,
      std::optional<std::string> _name,
      int _minLimitBytes,
      std::optional<int> _headroomLimitBytes,
      std::optional<int> _resumeOffsetBytes,
      std::string _bufferPoolName,
      std::optional<BufferPoolCfgPtr> _bufferPoolConfigPtr = std::nullopt)
      : id(_id),
        scalingFactor(_scalingFactor),
        name(_name),
        minLimitBytes(_minLimitBytes),
        headroomLimitBytes(_headroomLimitBytes),
        resumeOffsetBytes(_resumeOffsetBytes),
        bufferPoolName(_bufferPoolName),
        bufferPoolConfigPtr(_bufferPoolConfigPtr) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  state::PortPgFields toThrift() const override;
  static PortPgFields fromThrift(state::PortPgFields const&);

  uint8_t id{0};
  std::optional<cfg::MMUScalingFactor> scalingFactor{std::nullopt};
  std::optional<std::string> name{std::nullopt};
  int minLimitBytes{0};
  std::optional<int> headroomLimitBytes{std::nullopt};
  std::optional<int> resumeOffsetBytes{std::nullopt};
  std::string bufferPoolName;
  std::optional<BufferPoolCfgPtr> bufferPoolConfigPtr;
};

/*
 * PortPgConfig defines the behaviour of the per port queues
 */
class PortPgConfig
    : public ThriftyBaseT<state::PortPgFields, PortPgConfig, PortPgFields> {
 public:
  explicit PortPgConfig(uint8_t id) {
    writableFields()->id = id;
  }

  bool operator==(const PortPgConfig& portPg) const {
    return getFields()->id == portPg.getID() &&
        getFields()->minLimitBytes == portPg.getMinLimitBytes() &&
        getFields()->scalingFactor == portPg.getScalingFactor() &&
        getFields()->name == portPg.getName() &&
        getFields()->headroomLimitBytes == portPg.getHeadroomLimitBytes() &&
        getFields()->resumeOffsetBytes == portPg.getResumeOffsetBytes() &&
        getFields()->bufferPoolName == portPg.getBufferPoolName();
  }
  bool operator!=(const PortPgConfig& portPg) const {
    return !(*this == portPg);
  }

  // std::string toString() const;

  uint8_t getID() const {
    return getFields()->id;
  }

  int getMinLimitBytes() const {
    return getFields()->minLimitBytes;
  }

  void setMinLimitBytes(int minLimitBytes) {
    writableFields()->minLimitBytes = minLimitBytes;
  }

  std::optional<cfg::MMUScalingFactor> getScalingFactor() const {
    return getFields()->scalingFactor;
  }

  void setScalingFactor(cfg::MMUScalingFactor scalingFactor) {
    writableFields()->scalingFactor = scalingFactor;
  }

  std::optional<std::string> getName() const {
    return getFields()->name;
  }

  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  std::optional<int> getHeadroomLimitBytes() const {
    return getFields()->headroomLimitBytes;
  }

  void setHeadroomLimitBytes(int headroomLimitBytes) {
    writableFields()->headroomLimitBytes = headroomLimitBytes;
  }

  std::optional<int> getResumeOffsetBytes() const {
    return getFields()->resumeOffsetBytes;
  }

  void setResumeOffsetBytes(int resumeOffsetBytes) {
    writableFields()->resumeOffsetBytes = resumeOffsetBytes;
  }

  std::string getBufferPoolName() const {
    return getFields()->bufferPoolName;
  }

  void setBufferPoolName(const std::string& name) {
    writableFields()->bufferPoolName = name;
  }

  std::optional<BufferPoolCfgPtr> getBufferPoolConfig() const {
    return getFields()->bufferPoolConfigPtr;
  }

  void setBufferPoolConfig(BufferPoolCfgPtr bufferPoolConfigPtr) {
    writableFields()->bufferPoolConfigPtr = bufferPoolConfigPtr;
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::PortPgFields, PortPgConfig, PortPgFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};

using PortPgConfigs = std::vector<std::shared_ptr<PortPgConfig>>;
} // namespace facebook::fboss
