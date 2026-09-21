// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchStats;

class InterfaceStats {
 public:
  InterfaceStats(
      InterfaceID intfID,
      std::string intfName,
      SwitchStats* switchStats);
  ~InterfaceStats();

  void sentRouterAdvertisement();

  std::string getCounterKey(const std::string& key);

 private:
  InterfaceStats(InterfaceStats const&) = delete;
  InterfaceStats& operator=(InterfaceStats const&) = delete;

  void clearCounters();

  InterfaceID intfID_;
  std::string intfName_;
  SwitchStats* switchStats_;
};

} // namespace facebook::fboss
