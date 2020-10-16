/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/LoggingUtil.h"

#include "fboss/lib/TupleUtils.h"

#include <folly/Singleton.h>

extern "C" {
#include <sai.h>
}

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::SaiApiTable;
static folly::Singleton<SaiApiTable, singleton_tag_type> saiApiTableSingleton{};
std::shared_ptr<SaiApiTable> SaiApiTable::getInstance() {
  return saiApiTableSingleton.try_get();
}

namespace facebook::fboss {

void SaiApiTable::queryApis() {
  if (apisQueried_) {
    return;
  }
  apisQueried_ = true;
  std::get<std::unique_ptr<AclApi>>(apis_) = std::make_unique<AclApi>();
  std::get<std::unique_ptr<BridgeApi>>(apis_) = std::make_unique<BridgeApi>();
  std::get<std::unique_ptr<BufferApi>>(apis_) = std::make_unique<BufferApi>();
  std::get<std::unique_ptr<DebugCounterApi>>(apis_) =
      std::make_unique<DebugCounterApi>();
  std::get<std::unique_ptr<FdbApi>>(apis_) = std::make_unique<FdbApi>();
  std::get<std::unique_ptr<HashApi>>(apis_) = std::make_unique<HashApi>();
  std::get<std::unique_ptr<HostifApi>>(apis_) = std::make_unique<HostifApi>();
  std::get<std::unique_ptr<HashApi>>(apis_) = std::make_unique<HashApi>();
  std::get<std::unique_ptr<MplsApi>>(apis_) = std::make_unique<MplsApi>();
  std::get<std::unique_ptr<NextHopApi>>(apis_) = std::make_unique<NextHopApi>();
  std::get<std::unique_ptr<NextHopGroupApi>>(apis_) =
      std::make_unique<NextHopGroupApi>();
  std::get<std::unique_ptr<NeighborApi>>(apis_) =
      std::make_unique<NeighborApi>();
  std::get<std::unique_ptr<PortApi>>(apis_) = std::make_unique<PortApi>();
  std::get<std::unique_ptr<QosMapApi>>(apis_) = std::make_unique<QosMapApi>();
  std::get<std::unique_ptr<QueueApi>>(apis_) = std::make_unique<QueueApi>();
  std::get<std::unique_ptr<RouteApi>>(apis_) = std::make_unique<RouteApi>();
  std::get<std::unique_ptr<RouterInterfaceApi>>(apis_) =
      std::make_unique<RouterInterfaceApi>();
  std::get<std::unique_ptr<SchedulerApi>>(apis_) =
      std::make_unique<SchedulerApi>();
  std::get<std::unique_ptr<SwitchApi>>(apis_) = std::make_unique<SwitchApi>();
  std::get<std::unique_ptr<VirtualRouterApi>>(apis_) =
      std::make_unique<VirtualRouterApi>();
  std::get<std::unique_ptr<VlanApi>>(apis_) = std::make_unique<VlanApi>();
  std::get<std::unique_ptr<WredApi>>(apis_) = std::make_unique<WredApi>();
}

AclApi& SaiApiTable::aclApi() {
  return getApi<AclApi>();
}

const AclApi& SaiApiTable::aclApi() const {
  return getApi<AclApi>();
}

BridgeApi& SaiApiTable::bridgeApi() {
  return getApi<BridgeApi>();
}
const BridgeApi& SaiApiTable::bridgeApi() const {
  return getApi<BridgeApi>();
}

BufferApi& SaiApiTable::bufferApi() {
  return getApi<BufferApi>();
}
const BufferApi& SaiApiTable::bufferApi() const {
  return getApi<BufferApi>();
}

DebugCounterApi& SaiApiTable::debugCounterApi() {
  return getApi<DebugCounterApi>();
}
const DebugCounterApi& SaiApiTable::debugCounterApi() const {
  return getApi<DebugCounterApi>();
}

FdbApi& SaiApiTable::fdbApi() {
  return getApi<FdbApi>();
}
const FdbApi& SaiApiTable::fdbApi() const {
  return getApi<FdbApi>();
}

HashApi& SaiApiTable::hashApi() {
  return getApi<HashApi>();
}
const HashApi& SaiApiTable::hashApi() const {
  return getApi<HashApi>();
}

HostifApi& SaiApiTable::hostifApi() {
  return getApi<HostifApi>();
}
const HostifApi& SaiApiTable::hostifApi() const {
  return getApi<HostifApi>();
}
MplsApi& SaiApiTable::mplsApi() {
  return getApi<MplsApi>();
}
const MplsApi& SaiApiTable::mplsApi() const {
  return getApi<MplsApi>();
}
NextHopApi& SaiApiTable::nextHopApi() {
  return getApi<NextHopApi>();
}
const NextHopApi& SaiApiTable::nextHopApi() const {
  return getApi<NextHopApi>();
}

NextHopGroupApi& SaiApiTable::nextHopGroupApi() {
  return getApi<NextHopGroupApi>();
}
const NextHopGroupApi& SaiApiTable::nextHopGroupApi() const {
  return getApi<NextHopGroupApi>();
}

NeighborApi& SaiApiTable::neighborApi() {
  return getApi<NeighborApi>();
}
const NeighborApi& SaiApiTable::neighborApi() const {
  return getApi<NeighborApi>();
}

PortApi& SaiApiTable::portApi() {
  return getApi<PortApi>();
}
const PortApi& SaiApiTable::portApi() const {
  return getApi<PortApi>();
}

QosMapApi& SaiApiTable::qosMapApi() {
  return getApi<QosMapApi>();
}
const QosMapApi& SaiApiTable::qosMapApi() const {
  return getApi<QosMapApi>();
}

QueueApi& SaiApiTable::queueApi() {
  return getApi<QueueApi>();
}
const QueueApi& SaiApiTable::queueApi() const {
  return getApi<QueueApi>();
}

RouteApi& SaiApiTable::routeApi() {
  return getApi<RouteApi>();
}
const RouteApi& SaiApiTable::routeApi() const {
  return getApi<RouteApi>();
}

RouterInterfaceApi& SaiApiTable::routerInterfaceApi() {
  return getApi<RouterInterfaceApi>();
}
const RouterInterfaceApi& SaiApiTable::routerInterfaceApi() const {
  return getApi<RouterInterfaceApi>();
}

SchedulerApi& SaiApiTable::schedulerApi() {
  return getApi<SchedulerApi>();
}
const SchedulerApi& SaiApiTable::schedulerApi() const {
  return getApi<SchedulerApi>();
}

SwitchApi& SaiApiTable::switchApi() {
  return getApi<SwitchApi>();
}
const SwitchApi& SaiApiTable::switchApi() const {
  return getApi<SwitchApi>();
}

VirtualRouterApi& SaiApiTable::virtualRouterApi() {
  return getApi<VirtualRouterApi>();
}
const VirtualRouterApi& SaiApiTable::virtualRouterApi() const {
  return getApi<VirtualRouterApi>();
}

VlanApi& SaiApiTable::vlanApi() {
  return getApi<VlanApi>();
}
const VlanApi& SaiApiTable::vlanApi() const {
  return getApi<VlanApi>();
}

WredApi& SaiApiTable::wredApi() {
  return getApi<WredApi>();
}
const WredApi& SaiApiTable::wredApi() const {
  return getApi<WredApi>();
}

TamApi& SaiApiTable::tamApi() {
  return getApi<TamApi>();
}

const TamApi& SaiApiTable::tamApi() const {
  return getApi<TamApi>();
}

void SaiApiTable::enableLogging(const std::string& logLevelStr) const {
  auto logLevel = saiLogLevelFromString(logLevelStr);
  for (uint32_t api = SAI_API_UNSPECIFIED; api < SAI_API_MAX; api++) {
    auto rv = sai_log_set((sai_api_t)api, logLevel);
    // Log a error but continue on if we can't set log level
    // for a api type. A adaptor may not support debug logging
    // for a particular API (e.g. in cases where we have had
    // to add stubs for a yet to be supported API). Make this
    // a non fatal condition.
    saiLogError(rv, (sai_api_t)api, "Failed to set debug log for api");
  }
}
} // namespace facebook::fboss
