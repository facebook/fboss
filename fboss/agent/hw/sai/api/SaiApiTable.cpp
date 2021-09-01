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

template <typename SaiApiPtr>
sai_api_t getSaiApiType(SaiApiPtr& /* api */) {
  using SaiApiT = typename std::pointer_traits<SaiApiPtr>::element_type;
  return SaiApiT::ApiType;
}
} // namespace

using facebook::fboss::SaiApiTable;
static folly::Singleton<SaiApiTable, singleton_tag_type> saiApiTableSingleton{};
std::shared_ptr<SaiApiTable> SaiApiTable::getInstance() {
  return saiApiTableSingleton.try_get();
}

namespace facebook::fboss {

const std::set<sai_api_t>& SaiApiTable::getFullApiList() const {
  static std::set<sai_api_t> fullApiList;
  if (fullApiList.empty()) {
    // Generate fullApiList_ based on the ApiT in apis_ tuple
    tupleForEach(
        [this](auto& api) { fullApiList.insert(getSaiApiType(api)); }, apis_);
  }
  return fullApiList;
}

template <typename SaiApiPtr>
void SaiApiTable::initApiIfDesired(
    SaiApiPtr& /* api */,
    const std::set<sai_api_t>& desiredApis) {
  using SaiApiT = typename std::pointer_traits<SaiApiPtr>::element_type;
  if (auto it = desiredApis.find(SaiApiT::ApiType); it != desiredApis.end()) {
    std::get<std::unique_ptr<SaiApiT>>(apis_) = std::make_unique<SaiApiT>();
  }
}

void SaiApiTable::queryApis(
    sai_service_method_table_t* serviceMethodTable,
    const std::set<sai_api_t>& desiredApis) {
  if (apisQueried_) {
    return;
  }
  // Initialize the SAI API
  auto rv = sai_api_initialize(0, serviceMethodTable);
  saiCheckError(rv, "Unable to initialize sai api");
  apisQueried_ = true;
  const auto& apis = SaiApiTable::getInstance()->allApis();
  tupleForEach(
      [this, &desiredApis](auto& api) { initApiIfDesired(api, desiredApis); },
      apis);
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

MirrorApi& SaiApiTable::mirrorApi() {
  return getApi<MirrorApi>();
}
const MirrorApi& SaiApiTable::mirrorApi() const {
  return getApi<MirrorApi>();
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

SamplePacketApi& SaiApiTable::samplePacketApi() {
  return getApi<SamplePacketApi>();
}
const SamplePacketApi& SaiApiTable::samplePacketApi() const {
  return getApi<SamplePacketApi>();
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

LagApi& SaiApiTable::lagApi() {
  return getApi<LagApi>();
}

const LagApi& SaiApiTable::lagApi() const {
  return getApi<LagApi>();
}

MacsecApi& SaiApiTable::macsecApi() {
  return getApi<MacsecApi>();
}

const MacsecApi& SaiApiTable::macsecApi() const {
  return getApi<MacsecApi>();
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

HwWriteBehvaiorRAII::HwWriteBehvaiorRAII(HwWriteBehavior behavior) {
  const auto& apis = SaiApiTable::getInstance()->allApis();
  tupleForEach(
      [this, behavior](auto& api) {
        if (api) {
          previousApiBehavior_.emplace(
              std::make_pair(api->apiType(), api->getHwWriteBehavior()));
          api->setHwWriteBehavior(behavior);
        }
      },
      apis);
}

HwWriteBehvaiorRAII::~HwWriteBehvaiorRAII() {
  const auto& apis = SaiApiTable::getInstance()->allApis();
  tupleForEach(
      [this](auto& api) {
        if (api) {
          api->setHwWriteBehavior(
              previousApiBehavior_.find(api->apiType())->second);
        }
      },
      apis);
}
} // namespace facebook::fboss
