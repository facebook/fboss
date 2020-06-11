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

using SaiQosMap = SaiObject<SaiQosMapTraits>;

struct SaiQosMapHandle {
  std::shared_ptr<SaiQosMap> dscpQosMap;
  std::shared_ptr<SaiQosMap> tcQosMap;
};

class SaiQosMapManager {
 public:
  SaiQosMapManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  void addQosMap(const std::shared_ptr<QosPolicy>& newQosPolicy);
  void removeQosMap();
  void changeQosMap(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);

  SaiQosMapHandle* getQosMap();
  const SaiQosMapHandle* getQosMap() const;

 private:
  std::shared_ptr<SaiQosMap> setDscpQosMap(const DscpMap& newDscpMap);
  std::shared_ptr<SaiQosMap> setTcQosMap(
      const QosPolicy::TrafficClassToQueueId& newTcToQueueIdMap);
  void setQosMap(const std::shared_ptr<QosPolicy>& newQosPolicy);
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  SaiQosMapHandle* getQosMapImpl() const;
  // Only one QoS Map because we only manage the default, switch-wide, qos map
  std::unique_ptr<SaiQosMapHandle> handle_;
};

} // namespace facebook::fboss
