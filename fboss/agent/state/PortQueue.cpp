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
#include "fboss/agent/state/NodeBase-defs.h"
#include <folly/Conv.h>

namespace {
constexpr auto kAqm = "aqm";
constexpr auto kBehavior = "behavior";
constexpr auto kDetection = "detection";
constexpr auto kEarlyDrop = "earlyDrop";
constexpr auto kEcn = "ecn";
constexpr auto kId = "id";
constexpr auto kLinear = "linear";
constexpr auto kMaximumLength = "maximumLength";
constexpr auto kMinimumLength = "minimumLength";
constexpr auto kStreamType = "streamType";
constexpr auto kWeight = "weight";
constexpr auto kReservedBytes = "reserved";
constexpr auto kScalingFactor = "scalingFactor";
constexpr auto kScheduling = "scheduling";
}

namespace facebook { namespace fboss {

folly::dynamic PortQueueFields::toFollyDynamic() const {
  folly::dynamic queue = folly::dynamic::object;
  queue[kWeight] = weight;
  if (reservedBytes) {
    queue[kReservedBytes] = reservedBytes.value();
  }
  if (scalingFactor) {
    queue[kScalingFactor] =
      cfg::_MMUScalingFactor_VALUES_TO_NAMES.find(
          scalingFactor.value())->second;
  }
  queue[kId] = id;
  queue[kScheduling] =
    cfg::_QueueScheduling_VALUES_TO_NAMES.find(scheduling)->second;
  queue[kStreamType] =
    cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second;
  if (aqm) {
    queue[kAqm] = aqmToFollyDynamic();
  }
  return queue;
}

PortQueueFields PortQueueFields::fromFollyDynamic(
    const folly::dynamic& queueJson) {
  PortQueueFields queue(static_cast<uint8_t>(queueJson[kId].asInt()));

  cfg::StreamType streamType = cfg::_StreamType_NAMES_TO_VALUES.find(
      queueJson[kStreamType].asString().c_str())->second;
  queue.streamType = streamType;
  auto sched = cfg::_QueueScheduling_NAMES_TO_VALUES.find(
      queueJson[kScheduling].asString().c_str())->second;
  queue.scheduling = sched;
  if (queueJson.find(kReservedBytes) != queueJson.items().end()) {
    queue.reservedBytes = queueJson[kReservedBytes].asInt();
  }
  if (queueJson.find(kWeight) != queueJson.items().end()) {
    queue.weight = queueJson[kWeight].asInt();
  }
  if (queueJson.find(kScalingFactor) != queueJson.items().end()) {
    queue.scalingFactor = cfg::_MMUScalingFactor_NAMES_TO_VALUES.find(
        queueJson[kScalingFactor].asString().c_str())->second;
  }
  if (queueJson.find(kAqm) != queueJson.items().end()) {
    queue.aqm = aqmFromFollyDynamic(queueJson[kAqm]);
  }
  return queue;
}

folly::dynamic PortQueueFields::aqmToFollyDynamic() const {
  folly::dynamic aqmDynamic = folly::dynamic::object;
  aqmDynamic[kDetection] = folly::dynamic::object;
  aqmDynamic[kBehavior] = folly::dynamic::object;
  folly::dynamic detection = folly::dynamic::object;
  switch(aqm->detection.getType()) {
    case cfg::QueueCongestionDetection::Type::linear:
      detection[kMaximumLength] = aqm->detection.get_linear().maximumLength;
      detection[kMinimumLength] = aqm->detection.get_linear().minimumLength;
      aqmDynamic[kDetection][kLinear] = detection;
      break;
    case cfg::QueueCongestionDetection::Type::__EMPTY__:
    default:
      folly::assume_unreachable();
      break;
  }
  aqmDynamic[kBehavior][kEarlyDrop] = aqm->behavior.earlyDrop;
  aqmDynamic[kBehavior][kEcn] = aqm->behavior.ecn;
  return aqmDynamic;
}

cfg::ActiveQueueManagement PortQueueFields::aqmFromFollyDynamic(
    const folly::dynamic& aqmJson) {
  cfg::ActiveQueueManagement aqm;
  if (aqmJson.find(kDetection) == aqmJson.items().end()) {
    throw FbossError(
        "loaded Active Queue Management does not specify a congestion"
        " detection method");
  }
  if (aqmJson.find(kBehavior) == aqmJson.items().end()) {
    throw FbossError(
        "loaded Active Queue Management does not specify congested behavior");
  }
  if (aqmJson[kDetection].find(kLinear) != aqmJson[kDetection].items().end()) {
    cfg::LinearQueueCongestionDetection lqcd;
    if ((aqmJson[kDetection][kLinear].find(kMaximumLength) ==
         aqmJson[kDetection].items().end()) ||
        (aqmJson[kDetection][kLinear].find(kMinimumLength) ==
         aqmJson[kDetection][kLinear].items().end())) {
      throw FbossError(
          "loaded linear congestion detection does not specify a minimum and"
          " maximum congestion threshold");
    }
    lqcd.maximumLength = aqmJson[kDetection][kLinear][kMaximumLength].asInt();
    lqcd.minimumLength = aqmJson[kDetection][kLinear][kMinimumLength].asInt();
    aqm.detection.set_linear(lqcd);
  }
  if (aqmJson[kBehavior].find(kEarlyDrop) != aqmJson[kBehavior].items().end()) {
    aqm.behavior.earlyDrop = aqmJson[kBehavior][kEarlyDrop].asBool();
  }
  if (aqmJson[kBehavior].find(kEcn) != aqmJson[kBehavior].items().end()) {
    aqm.behavior.ecn = aqmJson[kBehavior][kEcn].asBool();
  }
  return aqm;
}

PortQueue::PortQueue(uint8_t id) : NodeBaseT(id) {
}

template class NodeBaseT<PortQueue, PortQueueFields>;

}} // facebook::fboss
