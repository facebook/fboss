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

#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/MatchToActionEntry.h"

namespace facebook { namespace fboss {

struct TrafficPolicyFields {
  TrafficPolicyFields() {}

  template<typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static TrafficPolicyFields fromFollyDynamic(const folly::dynamic& json);
  std::vector<std::shared_ptr<MatchToActionEntry>> matchToActions;
};

/*
 * TrafficPolicy stores state about how traffic should be policed and
 * directed
 */
class TrafficPolicy :
    public NodeBaseT<TrafficPolicy, TrafficPolicyFields> {
 public:
  TrafficPolicy();
  static std::shared_ptr<TrafficPolicy>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = TrafficPolicyFields::fromFollyDynamic(json);
    return std::make_shared<TrafficPolicy>(fields);
  }

  static std::shared_ptr<TrafficPolicy>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const TrafficPolicy& trafficPolicy) {
    if (getFields()->matchToActions.size() !=
        trafficPolicy.getMatchToActions().size()) {
      return false;
    }
    auto mtas = trafficPolicy.getMatchToActions();
    for (int i = 0; i < mtas.size(); i++) {
      if (!(*(getFields()->matchToActions[i]) == *(mtas[i]))) {
        return false;
      }
    }
    return true;
  }

  const std::vector<std::shared_ptr<MatchToActionEntry>>&
  getMatchToActions() const {
    return getFields()->matchToActions;
  }

  void resetMatchToActions(
      const std::vector<std::shared_ptr<MatchToActionEntry>>& matchToActions) {
    writableFields()->matchToActions.assign(matchToActions.begin(),
        matchToActions.end());
  }


 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
