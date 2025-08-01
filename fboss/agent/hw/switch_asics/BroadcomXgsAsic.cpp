// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
static constexpr int kDefaultMidPriCpuQueueId = 2;
static constexpr int kDefaultHiPriCpuQueueId = 9;
} // namespace

namespace facebook::fboss {

std::set<cfg::StreamType> BroadcomXgsAsic::getQueueStreamTypes(
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
      return {cfg::StreamType::MULTICAST};
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::MANAGEMENT_PORT:
      return {cfg::StreamType::UNICAST};
    case cfg::PortType::FABRIC_PORT:
    case cfg::PortType::RECYCLE_PORT:
    case cfg::PortType::EVENTOR_PORT:
      break;
  }
  throw FbossError(
      getAsicTypeStr(),
      " ASIC does not support:",
      apache::thrift::util::enumNameSafe(portType));
}

int BroadcomXgsAsic::getMidPriCpuQueueId() const {
  return kDefaultMidPriCpuQueueId;
}

int BroadcomXgsAsic::getHiPriCpuQueueId() const {
  return kDefaultHiPriCpuQueueId;
}

} // namespace facebook::fboss
