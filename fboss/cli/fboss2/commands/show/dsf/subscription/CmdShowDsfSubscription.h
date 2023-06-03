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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/dsf/CmdShowDsf.h"
#include "fboss/cli/fboss2/commands/show/dsf/subscription/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowDsfSubscriptionTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowDsf;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowDsfSubscriptionModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowDsfSubscription
    : public CmdHandler<CmdShowDsfSubscription, CmdShowDsfSubscriptionTraits> {
 public:
  using RetType = CmdShowDsfSubscriptionTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::vector<facebook::fboss::FsdbSubscriptionThrift> subscriptions;
    client->sync_getDsfSubscriptions(subscriptions);
    return createModel(subscriptions);
  }

  RetType createModel(
      std::vector<facebook::fboss::FsdbSubscriptionThrift> subscriptions) {
    RetType model;

    for (const auto& subscriptionThrift : subscriptions) {
      cli::Subscription subscription;
      subscription.name() = *subscriptionThrift.name();
      subscription.state() = *subscriptionThrift.state();
      subscription.paths() = *subscriptionThrift.paths();
      model.subscriptions()->push_back(subscription);
    }

    std::sort(
        model.subscriptions()->begin(),
        model.subscriptions()->end(),
        [](const auto& a, const auto& b) { return a.name() < b.name(); });
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    constexpr auto fmtString = "{:<30}{:<15}{:<45}\n";

    out << fmt::format(fmtString, "Name", "State", "Subscription Paths");

    for (const auto& entry : *model.subscriptions()) {
      out << fmt::format(
          fmtString,
          *entry.name(),
          *entry.state(),
          folly::join("; ", *entry.paths()));
    }
    out << std::endl;
  }
};
} // namespace facebook::fboss
