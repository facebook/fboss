/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/qos.h>
#include <bcm/types.h>
} // extern "C"

namespace facebook::fboss {
// wrapper for bcm_rx_cosq_mapping_extended_get
int BcmControlPlane::rxCosqMappingExtendedGet(
    int unit,
    bcm_rx_cosq_mapping_t* rx_cosq_mapping) {
  return bcm_rx_cosq_mapping_get(
      unit,
      rx_cosq_mapping->index,
      &rx_cosq_mapping->reasons,
      &rx_cosq_mapping->reasons_mask,
      &rx_cosq_mapping->int_prio,
      &rx_cosq_mapping->int_prio_mask,
      &rx_cosq_mapping->packet_type,
      &rx_cosq_mapping->packet_type_mask,
      &rx_cosq_mapping->cosq);
}

// wrapper for bcm_rx_cosq_mapping_extended_set
int BcmControlPlane::rxCosqMappingExtendedSet(
    int unit,
    int index,
    bcm_rx_reasons_t reasons,
    bcm_rx_reasons_t reasons_mask,
    uint8 int_prio,
    uint8 int_prio_mask,
    uint32 packet_type,
    uint32 packet_type_mask,
    bcm_cos_queue_t cosq) {
  return bcm_rx_cosq_mapping_set(
      unit,
      index,
      reasons,
      reasons_mask,
      int_prio,
      int_prio_mask, // internal priority match & mask
      packet_type,
      packet_type_mask, // packet type match & mask
      cosq);
}

// wrapper for bcm_rx_cosq_mapping_extended_delete
int BcmControlPlane::rxCosqMappingExtendedDelete(
    int unit,
    bcm_rx_cosq_mapping_t* rx_cosq_mapping) {
  return bcm_rx_cosq_mapping_delete(unit, rx_cosq_mapping->index);
}
} // namespace facebook::fboss
