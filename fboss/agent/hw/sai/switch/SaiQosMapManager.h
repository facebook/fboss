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

#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiQosMap = SaiObject<SaiQosMapTraits>;

struct SaiQosMapHandle {
  std::shared_ptr<SaiQosMap> dscpToTcMap;
  std::shared_ptr<SaiQosMap> tcToQueueMap;
  std::shared_ptr<SaiQosMap> expToTcMap;
  std::shared_ptr<SaiQosMap> tcToExpMap;
};

class SaiQosMapManager {
 public:
  SaiQosMapManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  void addQosMap(const std::shared_ptr<QosPolicy>& newQosPolicy);
  void removeQosMap();
  void changeQosMap(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);

  SaiQosMapHandle* getQosMap();
  const SaiQosMapHandle* getQosMap() const;

 private:
  std::shared_ptr<SaiQosMap> setDscpToTcQosMap(const DscpMap& newDscpMap);
  std::shared_ptr<SaiQosMap> setTcToQueueQosMap(
      const QosPolicy::TrafficClassToQueueId& newTcToQueueIdMap);
  std::shared_ptr<SaiQosMap> setExpToTcQosMap(const ExpMap& newExpMap);
  std::shared_ptr<SaiQosMap> setTcToExpQosMap(const ExpMap& newExpMap);
  void setQosMaps(const std::shared_ptr<QosPolicy>& newQosPolicy);

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  SaiQosMapHandle* getQosMapImpl() const;
  // Only one QoS Map because we only manage the default, switch-wide, qos map
  std::unique_ptr<SaiQosMapHandle> handle_;
};

} // namespace facebook::fboss
