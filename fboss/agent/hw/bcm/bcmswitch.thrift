namespace cpp2 facebook.fboss
namespace d neteng.fboss.bcmswitch
namespace py neteng.fboss.bcmswitch


const i32 STAT_UNINITIALIZED = -1


struct BcmHwTableStats {
  /**
  * Stale flag is set when for whatever reason
  * counter collection via BCM API fails. We log
  * errors and set stale counter to 1 in such scenarios.
  * This should never really happen in practice.
  **/
  1:  bool hw_table_stats_stale = true
  2:  i32 l3_host_max = STAT_UNINITIALIZED
  3:  i32 l3_host_used = STAT_UNINITIALIZED
  4:  i32 l3_host_free = STAT_UNINITIALIZED
  5:  i32 l3_ipv4_host_used = STAT_UNINITIALIZED
  6:  i32 l3_ipv6_host_used = STAT_UNINITIALIZED
  7:  i32 l3_nexthops_max = STAT_UNINITIALIZED
  8:  i32 l3_nexthops_used = STAT_UNINITIALIZED
  9:  i32 l3_nexthops_free = STAT_UNINITIALIZED
  10: i32 l3_ecmp_groups_max = STAT_UNINITIALIZED
  11: i32 l3_ecmp_groups_used = STAT_UNINITIALIZED
  12: i32 l3_ecmp_groups_free = STAT_UNINITIALIZED

  // LPM
  13: i32 lpm_ipv4_max = STAT_UNINITIALIZED
  14: i32 lpm_ipv4_used = STAT_UNINITIALIZED
  15: i32 lpm_ipv4_free = STAT_UNINITIALIZED
  16: i32 lpm_ipv6_mask_0_64_max = STAT_UNINITIALIZED
  17: i32 lpm_ipv6_mask_0_64_used = STAT_UNINITIALIZED
  18: i32 lpm_ipv6_mask_0_64_free = STAT_UNINITIALIZED
  19: i32 lpm_ipv6_mask_65_127_max = STAT_UNINITIALIZED
  20: i32 lpm_ipv6_mask_65_127_used = STAT_UNINITIALIZED
  21: i32 lpm_ipv6_mask_65_127_free = STAT_UNINITIALIZED
  22: i32 lpm_slots_max = STAT_UNINITIALIZED
  23: i32 lpm_slots_used = STAT_UNINITIALIZED
  24: i32 lpm_slots_free = STAT_UNINITIALIZED
}
