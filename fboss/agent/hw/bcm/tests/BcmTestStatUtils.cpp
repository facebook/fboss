/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/types.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/field.h>
#include <bcm/stat.h>
}

namespace facebook::fboss::utility {

uint64_t getAclInOutPackets(int unit, BcmAclStatHandle handle) {
  uint64_t value;
  bcm_field_stat_t type = bcmFieldStatPackets;
  auto rv = bcm_field_stat_sync_get(unit, handle, type, &value);
  bcmCheckError(rv, "Failed to update stat=", handle);
  return value;
}

uint64_t getQueueOutPackets(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq) {
  uint64_t value;
  auto rv =
      bcm_cosq_stat_sync_get(unit, gport, cosq, bcmCosqStatOutPackets, &value);
  bcmCheckError(rv, "failed to get queue stats");
  return value;
}

uint64_t getQueueOutPackets(
    bool useQueueGportForCos,
    int unit,
    int port,
    int queueId,
    bool isMulticast) {
  auto portGport = utility::getPortGport(unit, port);
  bcm_gport_t gport;
  bcm_cos_queue_t cosq;

  if (useQueueGportForCos) {
    utility::BcmCosToQueueMapper cosToQueueMapper(portGport);
    gport = isMulticast ? cosToQueueMapper.getMcastQueueForCos(queueId)
                        : cosToQueueMapper.getUcastQueueForCos(queueId);
    cosq = 0;
  } else {
    gport = portGport;
    cosq = queueId;
  }

  return utility::getQueueOutPackets(unit, gport, cosq);
}

} // namespace facebook::fboss::utility
