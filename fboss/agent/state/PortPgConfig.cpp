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
  portPg.id_ref() = id;
  portPg.minLimitBytes_ref() = minLimitBytes;
  if (headroomLimitBytes) {
    portPg.headroomLimitBytes_ref() = headroomLimitBytes.value();
  }
  if (resumeOffsetBytes) {
    portPg.resumeOffsetBytes_ref() = resumeOffsetBytes.value();
  }
  portPg.bufferPoolName_ref() = bufferPoolName;

  if (scalingFactor) {
    auto scalingFactorName = apache::thrift::util::enumName(*scalingFactor);
    if (scalingFactorName == nullptr) {
      CHECK(false) << "Unexpected MMU scaling factor: "
                   << static_cast<int>(*scalingFactor);
    }
    portPg.scalingFactor_ref() = scalingFactorName;
  }
  if (name) {
    portPg.name_ref() = name.value();
  }
  if (bufferPoolConfigPtr) {
    state::BufferPoolFields bufferPoolFields;
    portPg.bufferPoolConfig_ref() =
        (*bufferPoolConfigPtr)->getFields()->toThrift();
  }
  return portPg;
}

// static, public
PortPgFields PortPgFields::fromThrift(state::PortPgFields const& portPgThrift) {
  PortPgFields portPg;
  portPg.id = static_cast<uint8_t>(*portPgThrift.id_ref());
  portPg.minLimitBytes = portPgThrift.minLimitBytes_ref().value();

  if (portPgThrift.headroomLimitBytes_ref()) {
    portPg.headroomLimitBytes = portPgThrift.headroomLimitBytes_ref().value();
  }
  if (portPgThrift.resumeOffsetBytes_ref()) {
    portPg.resumeOffsetBytes = portPgThrift.resumeOffsetBytes_ref().value();
  }

  if (portPgThrift.scalingFactor_ref()) {
    cfg::MMUScalingFactor scalingFactor;
    if (!TEnumTraits<cfg::MMUScalingFactor>::findValue(
            portPgThrift.scalingFactor_ref()->c_str(), &scalingFactor)) {
      CHECK(false) << "Invalid MMU scaling factor: "
                   << portPgThrift.scalingFactor_ref()->c_str();
    }
    portPg.scalingFactor = scalingFactor;
  }
  if (portPgThrift.name_ref()) {
    portPg.name = portPgThrift.name_ref().value();
  }
  portPg.bufferPoolName = portPgThrift.bufferPoolName_ref().value();
  if (auto bufferPoolConfig = portPgThrift.bufferPoolConfig_ref()) {
    portPg.bufferPoolConfigPtr = std::make_shared<BufferPoolCfg>(
        BufferPoolCfgFields::fromThrift(*bufferPoolConfig));
  }
  return portPg;
}

template class NodeBaseT<PortPgConfig, PortPgFields>;

} // namespace facebook::fboss
