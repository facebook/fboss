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
