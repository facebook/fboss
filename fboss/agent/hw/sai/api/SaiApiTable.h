/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HashApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/api/MacsecApi.h"
#include "fboss/agent/hw/sai/api/MirrorApi.h"
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/SamplePacketApi.h"
#include "fboss/agent/hw/sai/api/SchedulerApi.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/api/WredApi.h"

#include <memory>

namespace facebook::fboss {

class SaiApiTable {
 public:
  SaiApiTable() = default;
  ~SaiApiTable();
  SaiApiTable(const SaiApiTable& other) = delete;
  SaiApiTable& operator=(const SaiApiTable& other) = delete;

  // Static function for getting the SaiApiTable folly::Singleton
  static std::shared_ptr<SaiApiTable> getInstance();

  /*
   * This constructs each individual SAI API object based on the input apis list
   * which queries the Adapter for the api implementation and thus renders them
   * ready for use by the Adapter Host.
   */
  void queryApis(
      sai_service_method_table_t* serviceMethodTable,
      const std::set<sai_api_t>& desiredApis);

  const AclApi& aclApi() const;

  const BridgeApi& bridgeApi() const;

  const BufferApi& bufferApi() const;

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  const CounterApi& counterApi() const;
#endif

  const DebugCounterApi& debugCounterApi() const;

  const FdbApi& fdbApi() const;

  const HashApi& hashApi() const;

  const HostifApi& hostifApi() const;

  const MirrorApi& mirrorApi() const;

  const MplsApi& mplsApi() const;

  const NextHopApi& nextHopApi() const;

  const NextHopGroupApi& nextHopGroupApi() const;

  const NeighborApi& neighborApi() const;

  const PortApi& portApi() const;

  const QosMapApi& qosMapApi() const;

  const QueueApi& queueApi() const;

  const RouteApi& routeApi() const;

  const RouterInterfaceApi& routerInterfaceApi() const;

  const SamplePacketApi& samplePacketApi() const;

  const SchedulerApi& schedulerApi() const;

  const SwitchApi& switchApi() const;

  const SystemPortApi& systemPortApi() const;

#if !defined(TAJO_SDK)
  const UdfApi& udfApi() const;
#endif

  const VirtualRouterApi& virtualRouterApi() const;

  const VlanApi& vlanApi() const;

  const WredApi& wredApi() const;

  const TamApi& tamApi() const;

  const TunnelApi& tunnelApi() const;

  const LagApi& lagApi() const;

  const MacsecApi& macsecApi() const;

  template <typename SaiApiT>
  SaiApiT& getApi() {
    return *std::get<std::unique_ptr<SaiApiT>>(apis_);
  }

  template <typename SaiApiT>
  const SaiApiT& getApi() const {
    return *std::get<std::unique_ptr<SaiApiT>>(apis_);
  }

  void enableLogging(const std::string& logLevelStr) const;
  const auto& allApis() const {
    return apis_;
  }

  const std::set<sai_api_t>& getFullApiList() const;

 private:
  std::tuple<
      std::unique_ptr<AclApi>,
      std::unique_ptr<BridgeApi>,
      std::unique_ptr<BufferApi>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      std::unique_ptr<CounterApi>,
#endif
      std::unique_ptr<DebugCounterApi>,
      std::unique_ptr<FdbApi>,
      std::unique_ptr<HashApi>,
      std::unique_ptr<HostifApi>,
      std::unique_ptr<NextHopApi>,
      std::unique_ptr<NextHopGroupApi>,
      std::unique_ptr<MirrorApi>,
      std::unique_ptr<MplsApi>,
      std::unique_ptr<NeighborApi>,
      std::unique_ptr<PortApi>,
      std::unique_ptr<QosMapApi>,
      std::unique_ptr<QueueApi>,
      std::unique_ptr<RouteApi>,
      std::unique_ptr<RouterInterfaceApi>,
      std::unique_ptr<SamplePacketApi>,
      std::unique_ptr<SchedulerApi>,
      std::unique_ptr<SwitchApi>,
      std::unique_ptr<SystemPortApi>,
#if !defined(TAJO_SDK)
      std::unique_ptr<UdfApi>,
#endif
      std::unique_ptr<VirtualRouterApi>,
      std::unique_ptr<VlanApi>,
      std::unique_ptr<WredApi>,
      std::unique_ptr<TamApi>,
      std::unique_ptr<TunnelApi>,
      std::unique_ptr<LagApi>,
      std::unique_ptr<MacsecApi>>
      apis_;
  bool apisQueried_{false};

  template <typename SaiApiPtr>
  void initApiIfDesired(SaiApiPtr& api, const std::set<sai_api_t>& desiredApis);
};

} // namespace facebook::fboss
