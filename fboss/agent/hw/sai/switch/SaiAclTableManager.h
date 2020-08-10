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

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiAclTable = SaiObject<SaiAclTableTraits>;
using SaiAclEntry = SaiObject<SaiAclEntryTraits>;
using SaiAclCounter = SaiObject<SaiAclCounterTraits>;

struct SaiAclEntryHandle {
  /*
   * In FBOSS implementation, an ACL counter is always associated with single
   * ACL Entry. Thus, it is part of SaiAclEntryHandle.
   * ACL Counter is created first and then associated with an ACL Entry.
   * Thus, an ACL counter must be disassociated from ACL Entry before deleting
   * else the deletion fails with SAI_STATUS_OBJECT_IN_USE.
   *
   * Declare SaiAclCounter before SaiAclEntry as class members are destructed
   * in the reverse order of declaration.
   */
  std::shared_ptr<SaiAclCounter> aclCounter;
  std::shared_ptr<SaiAclEntry> aclEntry;
};

struct SaiAclTableHandle {
  std::shared_ptr<SaiAclTable> aclTable;
  // SAI ACL priority to corresponding handle
  folly::F14FastMap<int, std::unique_ptr<SaiAclEntryHandle>> aclTableMembers;
};

class SaiAclTableManager {
 public:
  SaiAclTableManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  /*
   * TOS is 8-bits: 6-bit DSCP followed by 2-bit ECN.
   * DSCP Mask is for 6-bit DSCP.
   * Thus, match all mask is  0b111111 i.e. 0x3F.
   */
  static auto constexpr kDscpMask = 0x3F;

  /*
   * L4 Src/Dst Port Mask.
   * L4 Src/Dst Port is 16-bit.
   */
  static auto constexpr kL4PortMask = 0xFFFF;

  static auto constexpr kIpProtocolMask = 0xFF;
  static auto constexpr kTcpFlagsMask = 0x3F;
  // Mask is not applicable for given field
  static auto constexpr kMaskDontCare = 0;
  static auto constexpr kIcmpTypeMask = 0xFF;
  static auto constexpr kIcmpCodeMask = 0xFF;

  static const folly::MacAddress& kMacMask() {
    static const folly::MacAddress macMask{"FF:FF:FF:FF:FF:FF"};

    return macMask;
  }

  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, and then pass and process following
   * data type for {add, remove, changed}AclTable:
   * const std:shared_ptr<AclTable>&.
   */
  AclTableSaiId addAclTable(const std::string& aclTableName);
  void removeAclTable();
  void changedAclTable();

  const SaiAclTableHandle* FOLLY_NULLABLE
  getAclTableHandle(const std::string& aclTableName) const;
  SaiAclTableHandle* FOLLY_NULLABLE
  getAclTableHandle(const std::string& aclTableName);

  AclEntrySaiId addAclEntry(
      const std::shared_ptr<AclEntry>& addedAclEntry,
      const std::string& aclTableName);
  void removeAclEntry(
      const std::shared_ptr<AclEntry>& removedAclEntry,
      const std::string& aclTableName);
  void changedAclEntry(
      const std::shared_ptr<AclEntry>& oldAclEntry,
      const std::shared_ptr<AclEntry>& newAclEntry,
      const std::string& aclTableName);

  const SaiAclEntryHandle* FOLLY_NULLABLE getAclEntryHandle(
      const SaiAclTableHandle* aclTableHandle,
      int priority) const;

  std::shared_ptr<SaiAclCounter> addAclCounter(
      const SaiAclTableHandle* aclTableHandle,
      const cfg::TrafficCounter& trafficCount);

  sai_uint32_t swPriorityToSaiPriority(int priority) const;

  sai_acl_ip_frag_t cfgIpFragToSaiIpFrag(cfg::IpFragMatch cfgType) const;
  sai_acl_ip_type_t cfgIpTypeToSaiIpType(cfg::IpType cfgIpType) const;

  std::pair<sai_uint32_t, sai_uint32_t> cfgLookupClassToSaiFdbMetaDataAndMask(
      cfg::AclLookupClass lookupClass) const;
  std::pair<sai_uint32_t, sai_uint32_t> cfgLookupClassToSaiRouteMetaDataAndMask(
      cfg::AclLookupClass lookupClass) const;
  std::pair<sai_uint32_t, sai_uint32_t>
  cfgLookupClassToSaiNeighborMetaDataAndMask(
      cfg::AclLookupClass lookupClass) const;

 private:
  SaiAclTableHandle* FOLLY_NULLABLE
  getAclTableHandleImpl(const std::string& aclTableName) const;

  std::pair<
      SaiAclTableTraits::AdapterHostKey,
      SaiAclTableTraits::CreateAttributes>
  createAclTableV4AndV6Helper(bool isV4);

  std::pair<
      SaiAclTableTraits::AdapterHostKey,
      SaiAclTableTraits::CreateAttributes>
  createAclTableHelper();

  sai_u32_range_t getFdbDstUserMetaDataRange() const;
  sai_u32_range_t getRouteDstUserMetaDataRange() const;
  sai_u32_range_t getNeighborDstUserMetaDataRange() const;

  sai_uint32_t getMetaDataMask(sai_uint32_t metaDataMax) const;

  sai_uint32_t cfgLookupClassToSaiMetaDataAndMaskHelper(
      cfg::AclLookupClass lookupClass,
      sai_uint32_t dstUserMetaDataRangeMin,
      sai_uint32_t dstUserMetaDataRangeMax) const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  // SAI ACL Table name to corresponding Handle
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, then use name from AclTable as key.
   */
  using SaiAclTableHandles =
      folly::F14FastMap<std::string, std::unique_ptr<SaiAclTableHandle>>;
  SaiAclTableHandles handles_;

  const sai_uint32_t aclEntryMinimumPriority_;
  const sai_uint32_t aclEntryMaximumPriority_;

  const sai_uint32_t fdbDstUserMetaDataRangeMin_;
  const sai_uint32_t fdbDstUserMetaDataRangeMax_;
  const sai_uint32_t fdbDstUserMetaDataMask_;

  const sai_uint32_t routeDstUserMetaDataRangeMin_;
  const sai_uint32_t routeDstUserMetaDataRangeMax_;
  const sai_uint32_t routeDstUserMetaDataMask_;

  const sai_uint32_t neighborDstUserMetaDataRangeMin_;
  const sai_uint32_t neighborDstUserMetaDataRangeMax_;
  const sai_uint32_t neighborDstUserMetaDataMask_;
};

} // namespace facebook::fboss
