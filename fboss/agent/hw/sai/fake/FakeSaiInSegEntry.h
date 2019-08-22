// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <saimpls.h>
}

#include <functional>

namespace facebook {
namespace fboss {

struct FakeSaiInSegEntry {
  sai_inseg_entry_t sai_inseg_entry;

  explicit FakeSaiInSegEntry(sai_inseg_entry_t data);
  bool operator==(const FakeSaiInSegEntry& other) const;
};

} // namespace fboss
} // namespace facebook

namespace std {

template <>
struct hash<facebook::fboss::FakeSaiInSegEntry> {
  size_t operator()(const facebook::fboss::FakeSaiInSegEntry& key) const;
};

} // namespace std
