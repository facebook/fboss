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
#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HashApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/SchedulerApi.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/api/WredApi.h"

#include <memory>

namespace facebook::fboss {

class SaiApiTable {
 public:
  SaiApiTable() = default;
  ~SaiApiTable() = default;
  SaiApiTable(const SaiApiTable& other) = delete;
  SaiApiTable& operator=(const SaiApiTable& other) = delete;

  // Static function for getting the SaiApiTable folly::Singleton
  static std::shared_ptr<SaiApiTable> getInstance();

  /*
   * This constructs each individual SAI API object which queries the Adapter
   * for the api implementation and thus renders them ready for use by the
   * Adapter Host.
   */
  void queryApis();

  AclApi& aclApi();
  const AclApi& aclApi() const;

  BridgeApi& bridgeApi();
  const BridgeApi& bridgeApi() const;

  BufferApi& bufferApi();
  const BufferApi& bufferApi() const;

  DebugCounterApi& debugCounterApi();
  const DebugCounterApi& debugCounterApi() const;

  FdbApi& fdbApi();
  const FdbApi& fdbApi() const;

  HashApi& hashApi();
  const HashApi& hashApi() const;

  HostifApi& hostifApi();
  const HostifApi& hostifApi() const;

  MplsApi& mplsApi();
  const MplsApi& mplsApi() const;

  NextHopApi& nextHopApi();
  const NextHopApi& nextHopApi() const;

  NextHopGroupApi& nextHopGroupApi();
  const NextHopGroupApi& nextHopGroupApi() const;

  NeighborApi& neighborApi();
  const NeighborApi& neighborApi() const;

  PortApi& portApi();
  const PortApi& portApi() const;

  QosMapApi& qosMapApi();
  const QosMapApi& qosMapApi() const;

  QueueApi& queueApi();
  const QueueApi& queueApi() const;

  RouteApi& routeApi();
  const RouteApi& routeApi() const;

  RouterInterfaceApi& routerInterfaceApi();
  const RouterInterfaceApi& routerInterfaceApi() const;

  SchedulerApi& schedulerApi();
  const SchedulerApi& schedulerApi() const;

  SwitchApi& switchApi();
  const SwitchApi& switchApi() const;

  VirtualRouterApi& virtualRouterApi();
  const VirtualRouterApi& virtualRouterApi() const;

  VlanApi& vlanApi();
  const VlanApi& vlanApi() const;

  WredApi& wredApi();
  const WredApi& wredApi() const;

  TamApi& tamApi();
  const TamApi& tamApi() const;

  template <typename SaiApiT>
  SaiApiT& getApi() {
    return *std::get<std::unique_ptr<SaiApiT>>(apis_);
  }

  template <typename SaiApiT>
  const SaiApiT& getApi() const {
    return *std::get<std::unique_ptr<SaiApiT>>(apis_);
  }

  void enableLogging(const std::string& logLevelStr) const;

 private:
  std::tuple<
      std::unique_ptr<AclApi>,
      std::unique_ptr<BridgeApi>,
      std::unique_ptr<BufferApi>,
      std::unique_ptr<DebugCounterApi>,
      std::unique_ptr<FdbApi>,
      std::unique_ptr<HashApi>,
      std::unique_ptr<HostifApi>,
      std::unique_ptr<NextHopApi>,
      std::unique_ptr<NextHopGroupApi>,
      std::unique_ptr<MplsApi>,
      std::unique_ptr<NeighborApi>,
      std::unique_ptr<PortApi>,
      std::unique_ptr<QosMapApi>,
      std::unique_ptr<QueueApi>,
      std::unique_ptr<RouteApi>,
      std::unique_ptr<RouterInterfaceApi>,
      std::unique_ptr<SchedulerApi>,
      std::unique_ptr<SwitchApi>,
      std::unique_ptr<VirtualRouterApi>,
      std::unique_ptr<VlanApi>,
      std::unique_ptr<WredApi>,
      std::unique_ptr<TamApi>>
      apis_;
  bool apisQueried_{false};
};

} // namespace facebook::fboss
