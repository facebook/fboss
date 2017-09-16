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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/AclEntry.h"

namespace facebook { namespace fboss {

struct MatchToActionEntryFields {
  MatchToActionEntryFields() {}

  template<typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static MatchToActionEntryFields fromFollyDynamic(const folly::dynamic& json);
  std::shared_ptr<AclEntry> matcher;
  cfg::MatchAction action;
};

/*
 * MatchToActionEntry stores state about one of the access control entries on
 * the switch and the action that should be taken when matched.
 */
class MatchToActionEntry :
    public NodeBaseT<MatchToActionEntry, MatchToActionEntryFields> {
 public:
  MatchToActionEntry();
  static std::shared_ptr<MatchToActionEntry>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = MatchToActionEntryFields::fromFollyDynamic(json);
    return std::make_shared<MatchToActionEntry>(fields);
  }

  static std::shared_ptr<MatchToActionEntry>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const MatchToActionEntry& mtae) {
    return *(getFields()->matcher) == *(mtae.getMatcher()) &&
      getFields()->action == mtae.getAction();
  }

  const std::shared_ptr<AclEntry>& getMatcher() const {
    return getFields()->matcher;
  }

  void setMatcher(std::shared_ptr<AclEntry> matcher) {
    writableFields()->matcher.swap(matcher);
  }

  cfg::MatchAction getAction() const {
    return getFields()->action;
  }

  void setAction(const cfg::MatchAction& action) {
    writableFields()->action = action;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
