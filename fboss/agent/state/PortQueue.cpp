/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortQueue.h"
#include <folly/Conv.h>
#include <sstream>
#include "fboss/agent/state/NodeBase-defs.h"

namespace {
template <typename Param>
bool isPortQueueOptionalAttributeSame(
    const std::optional<Param>& swValue,
    bool isConfSet,
    const Param& confValue) {
  if (!swValue.has_value() && !isConfSet) {
    return true;
  }
  if (swValue.has_value() && isConfSet && swValue.value() == confValue) {
    return true;
  }
  return false;
}

bool comparePortQueueAQMs(
    const facebook::fboss::PortQueue::AQMMap& aqmMap,
    const std::vector<facebook::fboss::cfg::ActiveQueueManagement>& aqms) {
  auto sortedAqms = aqms;
  std::sort(
      sortedAqms.begin(),
      sortedAqms.end(),
      [](const auto& lhs, const auto& rhs) {
        return lhs.behavior < rhs.behavior;
      });
  return std::equal(
      aqmMap.begin(),
      aqmMap.end(),
      sortedAqms.begin(),
      sortedAqms.end(),
      [](const auto& behaviorAndAqm, const auto& aqm) {
        return behaviorAndAqm.second == aqm;
      });
}
} // unnamed namespace

namespace facebook::fboss {

state::PortQueueFields PortQueueFields::toThrift() const {
  state::PortQueueFields queue;
  *queue.id_ref() = id;
  *queue.weight_ref() = weight;
  if (reservedBytes) {
    queue.reserved_ref() = reservedBytes.value();
  }
  if (scalingFactor) {
    queue.scalingFactor_ref() =
        cfg::_MMUScalingFactor_VALUES_TO_NAMES.at(*scalingFactor);
  }
  *queue.scheduling_ref() =
      cfg::_QueueScheduling_VALUES_TO_NAMES.at(scheduling);
  *queue.streamType_ref() = cfg::_StreamType_VALUES_TO_NAMES.at(streamType);
  if (name) {
    queue.name_ref() = name.value();
  }
  if (sharedBytes) {
    queue.sharedBytes_ref() = sharedBytes.value();
  }
  if (!aqms.empty()) {
    std::vector<cfg::ActiveQueueManagement> aqmList;
    for (const auto& aqm : aqms) {
      aqmList.push_back(aqm.second);
    }
    queue.aqms_ref() = aqmList;
  }

  if (portQueueRate) {
    queue.portQueueRate_ref() = portQueueRate.value();
  }

  if (bandwidthBurstMinKbits) {
    queue.bandwidthBurstMinKbits_ref() = bandwidthBurstMinKbits.value();
  }

  if (bandwidthBurstMaxKbits) {
    queue.bandwidthBurstMaxKbits_ref() = bandwidthBurstMaxKbits.value();
  }

  if (trafficClass) {
    queue.trafficClass_ref() = static_cast<int16_t>(trafficClass.value());
  }

  return queue;
}

// static, public
PortQueueFields PortQueueFields::fromThrift(
    state::PortQueueFields const& queueThrift) {
  PortQueueFields queue;
  queue.id = static_cast<uint8_t>(*queueThrift.id_ref());

  auto const itrStreamType = cfg::_StreamType_NAMES_TO_VALUES.find(
      queueThrift.streamType_ref()->c_str());
  CHECK(itrStreamType != cfg::_StreamType_NAMES_TO_VALUES.end());
  queue.streamType = itrStreamType->second;

  auto const itrSched = cfg::_QueueScheduling_NAMES_TO_VALUES.find(
      queueThrift.scheduling_ref()->c_str());
  CHECK(itrSched != cfg::_QueueScheduling_NAMES_TO_VALUES.end());
  queue.scheduling = itrSched->second;

  queue.weight = *queueThrift.weight_ref();
  if (queueThrift.reserved_ref()) {
    queue.reservedBytes = queueThrift.reserved_ref().value();
  }
  if (queueThrift.scalingFactor_ref()) {
    auto itrScalingFactor = cfg::_MMUScalingFactor_NAMES_TO_VALUES.find(
        queueThrift.scalingFactor_ref()->c_str());
    CHECK(itrScalingFactor != cfg::_MMUScalingFactor_NAMES_TO_VALUES.end());
    queue.scalingFactor = itrScalingFactor->second;
  }
  if (queueThrift.name_ref()) {
    queue.name = queueThrift.name_ref().value();
  }
  if (queueThrift.sharedBytes_ref()) {
    queue.sharedBytes = queueThrift.sharedBytes_ref().value();
  }
  if (queueThrift.aqms_ref()) {
    for (const auto& aqm : queueThrift.aqms_ref().value()) {
      queue.aqms.emplace(aqm.behavior, aqm);
    }
  }

  if (queueThrift.portQueueRate_ref()) {
    queue.portQueueRate = queueThrift.portQueueRate_ref().value();
  }

  if (queueThrift.bandwidthBurstMinKbits_ref()) {
    queue.bandwidthBurstMinKbits =
        queueThrift.bandwidthBurstMinKbits_ref().value();
  }

  if (queueThrift.bandwidthBurstMaxKbits_ref()) {
    queue.bandwidthBurstMaxKbits =
        queueThrift.bandwidthBurstMaxKbits_ref().value();
  }

  if (queueThrift.trafficClass_ref()) {
    queue.trafficClass.emplace(
        static_cast<TrafficClass>(queueThrift.trafficClass_ref().value()));
  }

  return queue;
}

std::string PortQueue::toString() const {
  std::stringstream ss;
  ss << "Queue id=" << static_cast<int>(getID())
     << ", streamType=" << cfg::_StreamType_VALUES_TO_NAMES.at(getStreamType())
     << ", scheduling="
     << cfg::_QueueScheduling_VALUES_TO_NAMES.at(getScheduling())
     << ", weight=" << getWeight();
  if (getReservedBytes()) {
    ss << ", reservedBytes=" << getReservedBytes().value();
  }
  if (getSharedBytes()) {
    ss << ", sharedBytes=" << getSharedBytes().value();
  }
  if (getScalingFactor()) {
    ss << ", scalingFactor="
       << cfg::_MMUScalingFactor_VALUES_TO_NAMES.at(getScalingFactor().value());
  }
  if (!getAqms().empty()) {
    ss << ", aqms=[";
    for (const auto& aqm : getAqms()) {
      ss << "(behavior="
         << cfg::_QueueCongestionBehavior_VALUES_TO_NAMES.at(aqm.first)
         << ", detection=[min="
         << aqm.second.get_detection().get_linear().minimumLength
         << ", max=" << aqm.second.get_detection().get_linear().maximumLength
         << "]), ";
    }
    ss << "]";
  }
  if (getName()) {
    ss << ", name=" << getName().value();
  }

  if (getPortQueueRate()) {
    uint32_t rateMin, rateMax;
    std::string type;
    auto portQueueRate = getPortQueueRate().value();

    switch (portQueueRate.getType()) {
      case cfg::PortQueueRate::Type::pktsPerSec:
        type = "pps";
        rateMin = *portQueueRate.get_pktsPerSec().minimum_ref();
        rateMax = *portQueueRate.get_pktsPerSec().maximum_ref();
        break;
      case cfg::PortQueueRate::Type::kbitsPerSec:
        type = "pps";
        rateMin = *portQueueRate.get_kbitsPerSec().minimum_ref();
        rateMax = *portQueueRate.get_kbitsPerSec().maximum_ref();
        break;
      case cfg::PortQueueRate::Type::__EMPTY__:
        // needed to handle error from -Werror=switch, fall through
        FOLLY_FALLTHROUGH;
      default:
        type = "unknown";
        rateMin = 0;
        rateMax = 0;
        break;
    }

    ss << ", bandwidth " << type << " min: " << rateMin << " max: " << rateMax;
  }

  if (getBandwidthBurstMinKbits()) {
    ss << ", bandwidthBurstMinKbits: " << getBandwidthBurstMinKbits().value();
  }

  if (getBandwidthBurstMaxKbits()) {
    ss << ", bandwidthBurstMaxKbits: " << getBandwidthBurstMaxKbits().value();
  }

  return ss.str();
}

bool checkSwConfPortQueueMatch(
    const std::shared_ptr<PortQueue>& swQueue,
    const cfg::PortQueue* cfgQueue) {
  return swQueue->getID() == cfgQueue->id &&
      swQueue->getStreamType() == cfgQueue->streamType &&
      swQueue->getScheduling() == cfgQueue->scheduling &&
      (cfgQueue->scheduling == cfg::QueueScheduling::STRICT_PRIORITY ||
       swQueue->getWeight() == cfgQueue->weight_ref().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getReservedBytes(),
             (bool)cfgQueue->reservedBytes_ref(),
             cfgQueue->reservedBytes_ref().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getScalingFactor(),
             (bool)cfgQueue->scalingFactor_ref(),
             cfgQueue->scalingFactor_ref().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getSharedBytes(),
             (bool)cfgQueue->sharedBytes_ref().value_or({}),
             cfgQueue->sharedBytes_ref().value_or({})) &&
      comparePortQueueAQMs(
             swQueue->getAqms(), cfgQueue->aqms_ref().value_or({})) &&
      swQueue->getName() == cfgQueue->name_ref().value_or({}) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getPortQueueRate(),
             (bool)cfgQueue->portQueueRate_ref(),
             cfgQueue->portQueueRate_ref().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getBandwidthBurstMinKbits(),
             (bool)cfgQueue->bandwidthBurstMinKbits_ref(),
             cfgQueue->bandwidthBurstMinKbits_ref().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getBandwidthBurstMaxKbits(),
             (bool)cfgQueue->bandwidthBurstMaxKbits_ref(),
             cfgQueue->bandwidthBurstMaxKbits_ref().value_or({}));
}

template class NodeBaseT<PortQueue, PortQueueFields>;

} // namespace facebook::fboss
