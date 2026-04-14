// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

extern "C" {
#include <sai.h>
}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include <functional>

namespace facebook::fboss {

struct FakeSaiMySidEntry {
  sai_my_sid_entry_t sai_my_sid_entry{};

  explicit FakeSaiMySidEntry(sai_my_sid_entry_t data);
  bool operator==(const FakeSaiMySidEntry& other) const;
};

} // namespace facebook::fboss

namespace std {

template <>
struct hash<facebook::fboss::FakeSaiMySidEntry> {
  size_t operator()(const facebook::fboss::FakeSaiMySidEntry& key) const;
};

} // namespace std

#endif
