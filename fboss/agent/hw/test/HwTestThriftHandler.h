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

#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/gen-cpp2/switch_state_types.h"

#include <memory>

namespace facebook::fboss {
class HwSwitch;
}
namespace facebook::fboss::utility {
class HwTestThriftHandler : public AgentHwTestCtrlSvIf {
 public:
  explicit HwTestThriftHandler(HwSwitch* hwSwitch) : hwSwitch_(hwSwitch) {}

  int32_t getDefaultAclTableNumAclEntries() override;

  int32_t getAclTableNumAclEntries(std::unique_ptr<std::string> name) override;

  bool isDefaultAclTableEnabled() override;

  bool isAclTableEnabled(std::unique_ptr<std::string> name) override;

  bool isAclEntrySame(
      std::unique_ptr<state::AclEntryFields> aclEntry,
      std::unique_ptr<std::string> aclTableName) override;

  bool areAllAclEntriesEnabled() override;

  bool isStatProgrammedInDefaultAclTable(
      std::unique_ptr<std::vector<::std::string>> aclEntryNames,
      std::unique_ptr<std::string> counterName,
      std::unique_ptr<std::vector<cfg::CounterType>> types) override;

  bool isStatProgrammedInAclTable(
      std::unique_ptr<std::vector<::std::string>> aclEntryNames,
      std::unique_ptr<std::string> counterName,
      std::unique_ptr<std::vector<cfg::CounterType>> types,
      std::unique_ptr<std::string> tableName) override;

  bool isMirrorProgrammed(std::unique_ptr<state::MirrorFields> mirror) override;

  bool isPortMirrored(
      int32_t port,
      std::unique_ptr<std::string> mirror,
      bool ingress) override;

  bool isPortSampled(
      int32_t port,
      std::unique_ptr<std::string> mirror,
      bool ingress) override;

  bool isAclEntryMirrored(
      std::unique_ptr<std::string> aclEntry,
      std::unique_ptr<std::string> mirror,
      bool ingress) override;
  void getNeighborInfo(
      NeighborInfo& neighborInfo,
      std::unique_ptr<::facebook::fboss::IfAndIP> neighbor) override;

  int getHwEcmpSize(
      std::unique_ptr<CIDRNetwork> prefix,
      int routerID,
      int sizeInSw) override;

  void injectFecError(
      std::unique_ptr<std::vector<int>> hwPorts,
      bool injectCorrectable) override;

  void injectSwitchReachabilityChangeNotification() override;

 private:
  HwSwitch* hwSwitch_;
};

std::shared_ptr<HwTestThriftHandler> createHwTestThriftHandler(HwSwitch* hw);
} // namespace facebook::fboss::utility
