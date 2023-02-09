/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortPgConfig.h"
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <sstream>
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeBase-defs.h"

using apache::thrift::TEnumTraits;

namespace facebook::fboss {

state::PortPgFields PortPgFields::toThrift() const {
  state::PortPgFields portPg;
  portPg.id() = id;
  portPg.minLimitBytes() = minLimitBytes;
  if (headroomLimitBytes) {
    portPg.headroomLimitBytes() = headroomLimitBytes.value();
  }
  if (resumeOffsetBytes) {
    portPg.resumeOffsetBytes() = resumeOffsetBytes.value();
  }
  portPg.bufferPoolName() = bufferPoolName;

  if (scalingFactor) {
    auto scalingFactorName = apache::thrift::util::enumName(*scalingFactor);
    if (scalingFactorName == nullptr) {
      CHECK(false) << "Unexpected MMU scaling factor: "
                   << static_cast<int>(*scalingFactor);
    }
    portPg.scalingFactor() = scalingFactorName;
  }
  if (name) {
    portPg.name() = name.value();
  }
  if (bufferPoolConfigPtr) {
    state::BufferPoolFields bufferPoolFields;
    portPg.bufferPoolConfig() = (*bufferPoolConfigPtr)->getFields()->toThrift();
  }
  return portPg;
}

// static, public
PortPgFields PortPgFields::fromThrift(state::PortPgFields const& portPgThrift) {
  PortPgFields portPg;
  portPg.id = static_cast<uint8_t>(*portPgThrift.id());
  portPg.minLimitBytes = portPgThrift.minLimitBytes().value();

  if (portPgThrift.headroomLimitBytes()) {
    portPg.headroomLimitBytes = portPgThrift.headroomLimitBytes().value();
  }
  if (portPgThrift.resumeOffsetBytes()) {
    portPg.resumeOffsetBytes = portPgThrift.resumeOffsetBytes().value();
  }

  if (portPgThrift.scalingFactor()) {
    cfg::MMUScalingFactor scalingFactor;
    if (!TEnumTraits<cfg::MMUScalingFactor>::findValue(
            portPgThrift.scalingFactor()->c_str(), &scalingFactor)) {
      CHECK(false) << "Invalid MMU scaling factor: "
                   << portPgThrift.scalingFactor()->c_str();
    }
    portPg.scalingFactor = scalingFactor;
  }
  if (portPgThrift.name()) {
    portPg.name = portPgThrift.name().value();
  }
  portPg.bufferPoolName = portPgThrift.bufferPoolName().value();
  if (auto bufferPoolConfig = portPgThrift.bufferPoolConfig()) {
    portPg.bufferPoolConfigPtr =
        std::make_shared<BufferPoolCfg>(*bufferPoolConfig);
  }
  return portPg;
}

template class ThriftStructNode<PortPgConfig, state::PortPgFields>;

} // namespace facebook::fboss
