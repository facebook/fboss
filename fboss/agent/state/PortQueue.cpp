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
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <sstream>
#include "fboss/agent/state/NodeBase-defs.h"

using apache::thrift::TEnumTraits;

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

bool isEqual(
    std::vector<facebook::fboss::cfg::ActiveQueueManagement>&& lhs,
    std::vector<facebook::fboss::cfg::ActiveQueueManagement>&& rhs) {
  auto compare = [](const auto& left, const auto& right) {
    return std::tie(*left.behavior(), *left.detection()) <
        std::tie(*right.behavior(), *right.detection());
  };

  std::sort(std::begin(lhs), std::end(lhs), compare);
  std::sort(std::begin(rhs), std::end(rhs), compare);

  return std::equal(
      std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}

bool comparePortQueueAQMs(
    const facebook::fboss::PortQueue::AqmsType& stateAqms,
    std::vector<facebook::fboss::cfg::ActiveQueueManagement>&& aqms) {
  if (!stateAqms) {
    return aqms.empty();
  }
  if (aqms.empty()) {
    return !stateAqms || stateAqms->empty();
  }

  // THRIFT_COPY: maintain set instead of list in both state and config thrift
  auto stateAqmsThrift = stateAqms->toThrift();
  return isEqual(stateAqms->toThrift(), std::move(aqms));
}

bool comparePortQueueRate(
    const facebook::fboss::PortQueue::PortQueueRateType& statePortQueueRate,
    bool isConfSet,
    const facebook::fboss::cfg::PortQueueRate& rate) {
  if (!statePortQueueRate && !isConfSet) {
    return true;
  }
  // THRIFT_COPY
  if (statePortQueueRate && isConfSet &&
      statePortQueueRate->toThrift() == rate) {
    return true;
  }
  return false;
}
} // unnamed namespace

namespace facebook::fboss {

std::string PortQueue::toString() const {
  std::stringstream ss;
  ss << "Queue id=" << static_cast<int>(getID())
     << ", streamType=" << apache::thrift::util::enumName(getStreamType())
     << ", scheduling=" << apache::thrift::util::enumName(getScheduling())
     << ", weight=" << getWeight();
  if (getReservedBytes()) {
    ss << ", reservedBytes=" << getReservedBytes().value();
  }
  if (getSharedBytes()) {
    ss << ", sharedBytes=" << getSharedBytes().value();
  }
  if (getScalingFactor()) {
    ss << ", scalingFactor="
       << apache::thrift::util::enumName(getScalingFactor().value());
  }
  const auto& aqms = getAqms();
  if (aqms && !aqms->empty()) {
    ss << ", aqms=[";
    for (const auto& aqm : std::as_const(*aqms)) {
      const auto& behavior = aqm->cref<switch_config_tags::behavior>();
      const auto& detection = aqm->cref<switch_config_tags::detection>();

      ss << "(behavior=" << apache::thrift::util::enumName(behavior->toThrift())
         << ", detection=[min="
         << detection->cref<switch_config_tags::linear>()
                ->cref<switch_config_tags::minimumLength>()
                ->cref()
         << ", max="
         << detection->cref<switch_config_tags::linear>()
                ->cref<switch_config_tags::maximumLength>()
                ->cref();
      const auto& probability = detection->cref<switch_config_tags::linear>()
                                    ->cref<switch_config_tags::probability>();
      if (probability) {
        ss << ", probability=" << probability->cref();
      }
      ss << "]), ";
    }
    ss << "]";
  }
  if (getName()) {
    ss << ", name=" << getName().value();
  }

  const auto& portQueueRate = getPortQueueRate();
  if (portQueueRate) {
    uint32_t rateMin, rateMax;
    std::string type;

    switch (portQueueRate->type()) {
      case cfg::PortQueueRate::Type::pktsPerSec:
        type = "pps";
        rateMin = portQueueRate->cref<switch_config_tags::pktsPerSec>()
                      ->get<switch_config_tags::minimum>()
                      ->cref();
        rateMax = portQueueRate->cref<switch_config_tags::pktsPerSec>()
                      ->get<switch_config_tags::maximum>()
                      ->cref();
        break;
      case cfg::PortQueueRate::Type::kbitsPerSec:
        type = "pps";
        rateMin = portQueueRate->cref<switch_config_tags::kbitsPerSec>()
                      ->get<switch_config_tags::minimum>()
                      ->cref();
        rateMax = portQueueRate->cref<switch_config_tags::kbitsPerSec>()
                      ->get<switch_config_tags::maximum>()
                      ->cref();
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
  return swQueue->getID() == *cfgQueue->id() &&
      swQueue->getStreamType() == *cfgQueue->streamType() &&
      swQueue->getScheduling() == *cfgQueue->scheduling() &&
      (*cfgQueue->scheduling() == cfg::QueueScheduling::STRICT_PRIORITY ||
       swQueue->getWeight() == cfgQueue->weight().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getReservedBytes(),
             (bool)cfgQueue->reservedBytes(),
             cfgQueue->reservedBytes().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getScalingFactor(),
             (bool)cfgQueue->scalingFactor(),
             cfgQueue->scalingFactor().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getSharedBytes(),
             (bool)cfgQueue->sharedBytes().value_or({}),
             cfgQueue->sharedBytes().value_or({})) &&
      comparePortQueueAQMs(swQueue->getAqms(), cfgQueue->aqms().value_or({})) &&
      swQueue->getName() == cfgQueue->name().value_or({}) &&
      comparePortQueueRate(
             swQueue->getPortQueueRate(),
             (bool)cfgQueue->portQueueRate(),
             cfgQueue->portQueueRate().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getBandwidthBurstMinKbits(),
             (bool)cfgQueue->bandwidthBurstMinKbits(),
             cfgQueue->bandwidthBurstMinKbits().value_or({})) &&
      isPortQueueOptionalAttributeSame(
             swQueue->getBandwidthBurstMaxKbits(),
             (bool)cfgQueue->bandwidthBurstMaxKbits(),
             cfgQueue->bandwidthBurstMaxKbits().value_or({}));
}

std::optional<cfg::QueueCongestionDetection> PortQueue::findDetectionInAqms(
    cfg::QueueCongestionBehavior behavior) const {
  const auto& aqms = getAqms();
  if (!aqms) {
    return std::nullopt;
  }
  for (const auto& aqm : std::as_const(*aqms)) {
    if (aqm->get<switch_config_tags::behavior>()->toThrift() == behavior) {
      // THRIFT_COPY
      return aqm->get<switch_config_tags::detection>()->toThrift();
    }
  }
  return std::nullopt;
}

bool PortQueue::isAqmsSame(const PortQueue* other) const {
  if (!other) {
    return false;
  }
  const auto& thisAqms = getAqms();
  const auto& thatAqms = other->getAqms();
  if (thisAqms == nullptr && thatAqms == nullptr) {
    return true;
  } else if (thisAqms == nullptr) {
    return false;
  } else if (thatAqms == nullptr) {
    return false;
  }
  // THRIFT_COPY
  return isEqual(thisAqms->toThrift(), thatAqms->toThrift());
}

template class thrift_cow::ThriftStructNode<PortQueueFields>;

} // namespace facebook::fboss
