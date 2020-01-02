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

#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclStat.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

class BcmSwitch;

/**
 * A class to keep state related to acl entries in BcmSwitch
 */
class BcmAclTable {
 public:
  using FilterIterator = MapFilter<int, std::unique_ptr<BcmAclEntry>>;
  using Filter = MapFilter<int, std::unique_ptr<BcmAclEntry>>::Predicate;
  using FilterEntry = MapFilter<int, std::unique_ptr<BcmAclEntry>>::Entry;
  using FilterAction = std::function<void(const FilterEntry&)>;

  explicit BcmAclTable(BcmSwitch* hw) : hw_(hw) {}
  ~BcmAclTable() {}
  void processAddedAcl(const int groupId, const std::shared_ptr<AclEntry>& acl);
  void processRemovedAcl(const std::shared_ptr<AclEntry>& acl);
  void releaseAcls();

  // Throw exception if not found
  BcmAclEntry* getAcl(int priority) const;
  // return nullptr if not found
  BcmAclEntry* getAclIf(int priority) const;
  // Throw exception if not found
  BcmAclStat* getAclStat(const std::string& name) const;
  // return nullptr if not found
  BcmAclStat* getAclStatIf(const std::string& name) const;
  // return 0 if stat does not exist
  uint32_t getAclStatRefCount(const std::string& name) const;
  uint32_t getAclStatCount() const;

  BcmAclStat* incRefOrCreateBcmAclStat(
      const std::string& counterName,
      const std::vector<cfg::CounterType>& counterTypes,
      int gid);
  BcmAclStat* incRefOrCreateBcmAclStat(
      const std::string& counterName,
      const std::vector<cfg::CounterType>& counterTypes,
      BcmAclStatHandle statHandle);
  void derefBcmAclStat(const std::string& name);

  /* for every map entry which meets given predicate, execute given action */
  void forFilteredEach(Filter predicate, FilterAction action) const;

 private:
  using BcmAclEntryMap =
      boost::container::flat_map<int, std::unique_ptr<BcmAclEntry>>;
  using BcmAclStatMap = boost::container::
      flat_map<std::string, std::pair<std::unique_ptr<BcmAclStat>, uint32_t>>;

  BcmSwitch* hw_;
  BcmAclEntryMap aclEntryMap_;
  BcmAclStatMap aclStatMap_;
};

} // namespace facebook::fboss
