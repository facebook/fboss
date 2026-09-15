/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/dsf/subscription/CmdShowDsfSubscription.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

CmdShowDsfSubscription::RetType CmdShowDsfSubscription::queryClient(
    const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  std::vector<facebook::fboss::FsdbSubscriptionThrift> subscriptions;
  client->sync_getDsfSubscriptions(subscriptions);
  std::vector<facebook::fboss::DsfSessionThrift> sessions;
  client->sync_getDsfSessions(sessions);
  return createModel(subscriptions, sessions);
}

CmdShowDsfSubscription::RetType CmdShowDsfSubscription::createModel(
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
      const auto& session = sitr->second;
      subscription.state() =
          apache::thrift::util::enumNameSafe(*session.state());
      if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
        subscription.establishedSince() = utils::getPrettyElapsedTime(
            apache::thrift::can_throw(*session.lastEstablishedAt()));
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

void CmdShowDsfSubscription::printOutput(
    const RetType& model,
    std::ostream& out) {
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

std::string_view CmdShowDsfSubscriptionTraits::description() {
  return "Displays this switch's DSF FSDB subscriptions to its peer switches: each peer's name and address, the subscription state, how long it has been established, and the FSDB paths subscribed to. DSF-only — applies to DSF switches and returns data only in a DSF topology.";
}

CmdShowDsfSubscription::RetType CmdShowDsfSubscription::sampleModel() {
  RetType model;

  cli::Subscription sub1;
  sub1.name() = "edsw001::2001:db8:1::1";
  sub1.state() = "ESTABLISHED";
  sub1.establishedSince() = "66d 22h 52m 57s";
  sub1.paths() = {
      "agent/switchState/systemPortMaps",
      "agent/switchState/interfaceMaps",
      "agent/fsdbSubscriptions/rdsw010::2001:db8:2::1",
  };
  model.subscriptions()->push_back(sub1);

  cli::Subscription sub2;
  sub2.name() = "edsw002::2001:db8:1::2";
  sub2.state() = "ESTABLISHED";
  sub2.establishedSince() = "66d 22h 52m 57s";
  sub2.paths() = {
      "agent/switchState/systemPortMaps",
      "agent/switchState/interfaceMaps",
      "agent/fsdbSubscriptions/rdsw010::2001:db8:2::1",
  };
  model.subscriptions()->push_back(sub2);

  cli::Subscription sub3;
  sub3.name() = "edsw003::2001:db8:1::3";
  sub3.state() = "ESTABLISHED";
  sub3.establishedSince() = "65d 16h 21m 39s";
  sub3.paths() = {
      "agent/switchState/systemPortMaps",
      "agent/switchState/interfaceMaps",
      "agent/fsdbSubscriptions/rdsw010::2001:db8:2::1",
  };
  model.subscriptions()->push_back(sub3);

  return model;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowDsfSubscription, CmdShowDsfSubscriptionTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowDsfSubscription,
    CmdShowDsfSubscriptionTraits>::getValidFilters();
} // namespace facebook::fboss
