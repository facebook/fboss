/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StatPrinters.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace {
template <typename HwStatT>
folly::fbstring toStr(const HwStatT& stats) {
  folly::fbstring out;
  apache::thrift::Serializer<
      apache::thrift::SimpleJSONProtocolReader,
      apache::thrift::SimpleJSONProtocolWriter>
      serializer;
  serializer.serialize(stats, &out);
  return out;
}
} // namespace

namespace facebook::fboss {

void toAppend(const HwPortStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwSysPortStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwTrunkStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwResourceStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwAsicErrors& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwTeFlowStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const TeFlowStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const FabricReachabilityStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwBufferPoolStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const CpuPortStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwSwitchDropStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwSwitchDramStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwSwitchFb303GlobalStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwFlowletStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}

void toAppend(const phy::PhyInfo& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}

void toAppend(const AclStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}
void toAppend(const HwSwitchWatermarkStats& stats, folly::fbstring* result) {
  result->append(toStr(stats));
}

std::ostream& operator<<(std::ostream& os, const HwPortStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwSysPortStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwTrunkStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwResourceStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwAsicErrors& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwTeFlowStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const TeFlowStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(
    std::ostream& os,
    const FabricReachabilityStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwBufferPoolStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const CpuPortStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwSwitchDropStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwSwitchDramStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(
    std::ostream& os,
    const HwSwitchFb303GlobalStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const HwFlowletStats& stats) {
  os << toStr(stats);
  return os;
}

std::ostream& operator<<(std::ostream& os, const phy::PhyInfo& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(std::ostream& os, const AclStats& stats) {
  os << toStr(stats);
  return os;
}
std::ostream& operator<<(
    std::ostream& os,
    const HwSwitchWatermarkStats& stats) {
  os << toStr(stats);
  return os;
}
} // namespace facebook::fboss
