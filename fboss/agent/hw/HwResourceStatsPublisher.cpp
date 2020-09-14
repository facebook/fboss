/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwResourceStatsPublisher.h"

#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

void HwResourceStatPublisher::publish(const HwResourceStats& stats) const {
  fb303::fbData->setCounter(
      "hw_table_stats_stale", *stats.hw_table_stats_stale_ref() ? 1 : 0);
  fb303::fbData->setCounter("l3_host_max", *stats.l3_host_max_ref());
  fb303::fbData->setCounter("l3_host_used", *stats.l3_host_used_ref());
  fb303::fbData->setCounter("l3_host_free", *stats.l3_host_free_ref());
  fb303::fbData->setCounter("l3_nexthops_max", *stats.l3_nexthops_max_ref());
  fb303::fbData->setCounter("l3_nexthops_used", *stats.l3_nexthops_used_ref());
  fb303::fbData->setCounter("l3_nexthops_free", *stats.l3_nexthops_free_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_used", *stats.l3_ecmp_groups_used_ref());

  fb303::fbData->setCounter(
      "l3_ipv4_host_used", *stats.l3_ipv4_host_used_ref());
  fb303::fbData->setCounter(
      "l3_ipv6_host_used", *stats.l3_ipv6_host_used_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_max", *stats.l3_ecmp_groups_max_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_free", *stats.l3_ecmp_groups_free_ref());

  // LPM
  fb303::fbData->setCounter("lpm_ipv4_max", *stats.lpm_ipv4_max_ref());
  fb303::fbData->setCounter("lpm_ipv4_free", *stats.lpm_ipv4_free_ref());
  fb303::fbData->setCounter("lpm_ipv4_used", *stats.lpm_ipv4_used_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_max", *stats.lpm_ipv6_mask_0_64_max_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", *stats.lpm_ipv6_mask_0_64_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_used", *stats.lpm_ipv6_mask_0_64_used_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_max", *stats.lpm_ipv6_mask_65_127_max_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", *stats.lpm_ipv6_mask_0_64_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_free", *stats.lpm_ipv6_mask_65_127_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_used", *stats.lpm_ipv6_mask_65_127_used_ref());
  fb303::fbData->setCounter("lpm_table_max", *stats.lpm_slots_max_ref());
  fb303::fbData->setCounter("lpm_table_used", *stats.lpm_slots_used_ref());
  fb303::fbData->setCounter("lpm_table_free", *stats.lpm_slots_free_ref());
  // Acl
  fb303::fbData->setCounter("acl_entries_used", *stats.acl_entries_used_ref());
  fb303::fbData->setCounter("acl_entries_free", *stats.acl_entries_free_ref());
  fb303::fbData->setCounter("acl_entries_max", *stats.acl_entries_max_ref());
  fb303::fbData->setCounter(
      "acl_counters_used", *stats.acl_counters_used_ref());
  fb303::fbData->setCounter(
      "acl_counters_free", *stats.acl_counters_free_ref());
  fb303::fbData->setCounter("acl_counters_max", *stats.acl_counters_max_ref());
  fb303::fbData->setCounter("acl_meters_used", *stats.acl_meters_used_ref());
  fb303::fbData->setCounter("acl_meters_free", *stats.acl_meters_free_ref());
  fb303::fbData->setCounter("acl_meters_max", *stats.acl_meters_max_ref());
  // Mirrors
  fb303::fbData->setCounter("mirrors_used", *stats.mirrors_used_ref());
  fb303::fbData->setCounter("mirrors_free", *stats.mirrors_free_ref());
  fb303::fbData->setCounter("mirrors_max", *stats.mirrors_max_ref());
  fb303::fbData->setCounter("mirrors_span", *stats.mirrors_span_ref());
  fb303::fbData->setCounter("mirrors_erspan", *stats.mirrors_erspan_ref());
  fb303::fbData->setCounter("mirrors_sflow", *stats.mirrors_sflow_ref());
}

} // namespace facebook::fboss
