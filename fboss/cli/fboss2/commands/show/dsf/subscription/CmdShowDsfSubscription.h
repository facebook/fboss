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
#include "thrift/lib/cpp/util/EnumUtils.h"

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
    std::vector<facebook::fboss::DsfSessionThrift> sessions;
    client->sync_getDsfSessions(sessions);
    return createModel(subscriptions, sessions);
  }

  RetType createModel(
      const std::vector<facebook::fboss::FsdbSubscriptionThrift>& subscriptions,
      const std::vector<facebook::fboss::DsfSessionThrift>& sessions) {
    RetType model;

    std::map<std::string, DsfSessionThrift> remoteNode2DsfSession;
    std::for_each(
        sessions.begin(),
        sessions.end(),
        [&remoteNode2DsfSession](const auto& session) {
          remoteNode2DsfSession[*session.remoteName()] = session;
        });
    for (const auto& subscriptionThrift : subscriptions) {
      cli::Subscription subscription;
      subscription.name() = *subscriptionThrift.subscriptionId();
      auto sitr = remoteNode2DsfSession.find(*subscription.name());
      if (sitr != remoteNode2DsfSession.end()) {
        auto session = remoteNode2DsfSession.find(*subscription.name())->second;
        subscription.state() =
            apache::thrift::util::enumNameSafe(*session.state());
        if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
          subscription.establishedSince() =
              utils::getPrettyElapsedTime(*session.lastEstablishedAt());
        } else {
          subscription.establishedSince() = "--";
        }
      }
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
    constexpr auto fmtString = "{:<60}{:<25}{:<25}{:<45}\n";

    out << fmt::format(
        fmtString, "Name", "State", "Est Since", "Subscription Paths");

    for (const auto& entry : *model.subscriptions()) {
      out << fmt::format(
          fmtString,
          *entry.name(),
          *entry.state(),
          *entry.establishedSince(),
          folly::join("; ", *entry.paths()));
    }
    out << std::endl;
  }
};
} // namespace facebook::fboss
