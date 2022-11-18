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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/NodeMap.h"

namespace facebook::fboss {

using AclTableMapTraits = NodeMapTraits<std::string, AclTable>;
/*
 * A container for the set of tables.
 */
class AclTableMap : public NodeMapT<AclTableMap, AclTableMapTraits> {
 public:
  AclTableMap();
  ~AclTableMap() override;

  std::map<std::string, state::AclTableFields> toThrift() const;

  static std::shared_ptr<AclTableMap> fromThrift(
      std::map<std::string, state::AclTableFields> const& thriftMap);

  static std::shared_ptr<AclTableMap> createDefaultAclTableMap(
      const folly::dynamic& swJson);
  static std::shared_ptr<AclTableMap> createDefaultAclTableMapFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap);

  static std::shared_ptr<AclMap> getDefaultAclTableMap(
      std::map<std::string, state::AclTableFields> const& thriftMap);

  static const folly::dynamic& getAclTableMapName(
      const folly::dynamic& aclTableMapJson) {
    /*
     * The first entry in the entries vector should contain the INGRESS ACL
     * STAGE table group that we are interested in
     */
    if (aclTableMapJson[0].find(kAclTableMap) !=
        aclTableMapJson[0].items().end()) {
      return AclTable::getAclTableName(aclTableMapJson[0][kAclTableMap]);
    } else {
      throw FbossError(
          "aclTableGroups should contain atleast one entry with AclTableMap");
    }
  }

  bool operator==(const AclTableMap& aclTableMap) const {
    if (numTables() != aclTableMap.numTables()) {
      return false;
    }
    for (auto const& table : *this) {
      if (!aclTableMap.getTableIf(table->getID()) ||
          *(aclTableMap.getTable(table->getID())) != *table) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const AclTableMap& aclTableMap) const {
    return !(*this == aclTableMap);
  }

  const std::shared_ptr<AclTable>& getTable(
      const std::string& tableName) const {
    return getNode(tableName);
  }
  std::shared_ptr<AclTable> getTableIf(const std::string& tableName) const {
    return getNodeIf(tableName);
  }

  size_t numTables() const {
    return size();
  }

  AclTableMap* modify(std::shared_ptr<SwitchState>* state);

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void addTable(const std::shared_ptr<AclTable>& aclTable) {
    addNode(aclTable);
  }
  void removeTable(const std::string& tableName) {
    removeTable(getNode(tableName));
  }
  void removeTable(const std::shared_ptr<AclTable>& aclTable) {
    removeNode(aclTable);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
