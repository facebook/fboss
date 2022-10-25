// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

std::set<cfg::StreamType> BroadcomXgsAsic::getQueueStreamTypes(
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
      return {cfg::StreamType::MULTICAST};
    case cfg::PortType::INTERFACE_PORT:
      return {cfg::StreamType::UNICAST};
    case cfg::PortType::FABRIC_PORT:
    case cfg::PortType::RECYCLE_PORT:
      break;
  }
  throw FbossError(
      getAsicTypeStr(),
      " ASIC does not support:",
      apache::thrift::util::enumNameSafe(portType));
}
} // namespace facebook::fboss
