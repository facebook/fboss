// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/fake/FakeSaiInSegEntry.h"

#include <boost/functional/hash.hpp>

namespace facebook::fboss {

FakeSaiInSegEntry::FakeSaiInSegEntry(sai_inseg_entry_t other_sai_inseg_entry) {
  sai_inseg_entry.switch_id = other_sai_inseg_entry.switch_id;
  sai_inseg_entry.label = other_sai_inseg_entry.label;
}

bool FakeSaiInSegEntry::operator==(const FakeSaiInSegEntry& other) const {
  return sai_inseg_entry.switch_id == other.sai_inseg_entry.switch_id &&
      sai_inseg_entry.label == other.sai_inseg_entry.label;
}

} // namespace facebook::fboss

namespace std {

size_t hash<facebook::fboss::FakeSaiInSegEntry>::operator()(
    const facebook::fboss::FakeSaiInSegEntry& key) const {
  std::size_t seed = 0;
  boost::hash_combine(seed, boost::hash_value(key.sai_inseg_entry.switch_id));
  boost::hash_combine(seed, boost::hash_value(key.sai_inseg_entry.label));
  return seed;
}

} // namespace std
