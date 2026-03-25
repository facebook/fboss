// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/fake/FakeSaiMySidEntry.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include <boost/functional/hash.hpp>
#include <cstring>

namespace facebook::fboss {

FakeSaiMySidEntry::FakeSaiMySidEntry(
    sai_my_sid_entry_t other_sai_my_sid_entry) {
  sai_my_sid_entry.switch_id = other_sai_my_sid_entry.switch_id;
  sai_my_sid_entry.vr_id = other_sai_my_sid_entry.vr_id;
  sai_my_sid_entry.locator_block_len = other_sai_my_sid_entry.locator_block_len;
  sai_my_sid_entry.locator_node_len = other_sai_my_sid_entry.locator_node_len;
  sai_my_sid_entry.function_len = other_sai_my_sid_entry.function_len;
  sai_my_sid_entry.args_len = other_sai_my_sid_entry.args_len;
  std::memcpy(
      &sai_my_sid_entry.sid, &other_sai_my_sid_entry.sid, sizeof(sai_ip6_t));
}

bool FakeSaiMySidEntry::operator==(const FakeSaiMySidEntry& other) const {
  return sai_my_sid_entry.switch_id == other.sai_my_sid_entry.switch_id &&
      sai_my_sid_entry.vr_id == other.sai_my_sid_entry.vr_id &&
      sai_my_sid_entry.locator_block_len ==
      other.sai_my_sid_entry.locator_block_len &&
      sai_my_sid_entry.locator_node_len ==
      other.sai_my_sid_entry.locator_node_len &&
      sai_my_sid_entry.function_len == other.sai_my_sid_entry.function_len &&
      sai_my_sid_entry.args_len == other.sai_my_sid_entry.args_len &&
      std::memcmp(
          &sai_my_sid_entry.sid,
          &other.sai_my_sid_entry.sid,
          sizeof(sai_ip6_t)) == 0;
}

} // namespace facebook::fboss

namespace std {

size_t hash<facebook::fboss::FakeSaiMySidEntry>::operator()(
    const facebook::fboss::FakeSaiMySidEntry& key) const {
  std::size_t seed = 0;
  boost::hash_combine(seed, boost::hash_value(key.sai_my_sid_entry.switch_id));
  boost::hash_combine(seed, boost::hash_value(key.sai_my_sid_entry.vr_id));
  boost::hash_combine(
      seed, boost::hash_value(key.sai_my_sid_entry.locator_block_len));
  boost::hash_combine(
      seed, boost::hash_value(key.sai_my_sid_entry.locator_node_len));
  boost::hash_combine(
      seed, boost::hash_value(key.sai_my_sid_entry.function_len));
  boost::hash_combine(seed, boost::hash_value(key.sai_my_sid_entry.args_len));
  for (size_t i = 0; i < sizeof(sai_ip6_t); i++) {
    boost::hash_combine(seed, boost::hash_value(key.sai_my_sid_entry.sid[i]));
  }
  return seed;
}

} // namespace std

#endif
