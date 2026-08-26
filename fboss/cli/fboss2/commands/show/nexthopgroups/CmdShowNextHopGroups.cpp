// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/nexthopgroups/CmdShowNextHopGroups.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

namespace {

std::string getGroupName(const NextHopGroup& group) {
  auto name = apache::thrift::get_pointer(group.name());
  return name == nullptr ? "--" : *name;
}

std::string getProgrammedState(const NextHopGroup& group) {
  auto isProgrammed = apache::thrift::get_pointer(group.isProgrammed());
  if (isProgrammed == nullptr) {
    return "unknown";
  }
  return *isProgrammed ? "yes" : "no";
}

bool isNamedGroup(const NextHopGroup& group) {
  return apache::thrift::get_pointer(group.name()) != nullptr;
}

cli::ShowNextHopGroupsModel createNextHopGroupsModel(
    const std::vector<NextHopGroup>& nextHopGroups,
    bool namedOnly) {
  cli::ShowNextHopGroupsModel model;
  for (const auto& group : nextHopGroups) {
    if (namedOnly && !isNamedGroup(group)) {
      continue;
    }

    cli::NextHopGroupEntry entry;
    entry.name() = getGroupName(group);
    entry.isNamed() = isNamedGroup(group);
    entry.programmed() = getProgrammedState(group);

    for (const auto& nextHop : group.nexthops().value()) {
      cli::NextHopInfo nextHopInfo;
      show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
      entry.nextHops()->push_back(
          show::route::utils::getNextHopInfoStr(nextHopInfo));
    }

    model.nextHopGroups()->push_back(std::move(entry));
  }
  return model;
}

void printNextHopGroups(
    const cli::ShowNextHopGroupsModel& model,
    std::ostream& out) {
  for (const auto& group : model.nextHopGroups().value()) {
    out << fmt::format(
        "NextHopGroup: {}  Programmed: {}\n",
        group.name().value(),
        group.programmed().value());
    for (const auto& nextHop : group.nextHops().value()) {
      out << fmt::format("  {}\n", nextHop);
    }
  }
}

} // namespace

CmdShowNextHopGroups::RetType CmdShowNextHopGroups::queryClient(
    const HostInfo& hostInfo) {
  std::vector<NextHopGroup> nextHopGroups;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getNextHopGroups(nextHopGroups);
  return createModel(nextHopGroups);
}

CmdShowNextHopGroups::RetType CmdShowNextHopGroups::createModel(
    const std::vector<NextHopGroup>& nextHopGroups,
    bool namedOnly) {
  return createNextHopGroupsModel(nextHopGroups, namedOnly);
}

void CmdShowNextHopGroups::printOutput(
    const RetType& model,
    std::ostream& out) {
  printNextHopGroups(model, out);
}

CmdShowNamedNextHopGroups::RetType CmdShowNamedNextHopGroups::queryClient(
    const HostInfo& hostInfo) {
  std::vector<NextHopGroup> nextHopGroups;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  // Use the name-keyed API so groups that share a next-hop set (and thus a
  // single hardware NextHopSetId) are each listed by name, rather than being
  // collapsed into one entry as sync_getNextHopGroups would do.
  std::vector<std::string> names; // empty => return all named groups
  client->sync_getNamedNextHopGroups(nextHopGroups, names);
  return createModel(nextHopGroups);
}

CmdShowNamedNextHopGroups::RetType CmdShowNamedNextHopGroups::createModel(
    const std::vector<NextHopGroup>& nextHopGroups) {
  return createNextHopGroupsModel(nextHopGroups, true /* namedOnly */);
}

void CmdShowNamedNextHopGroups::printOutput(
    const RetType& model,
    std::ostream& out) {
  printNextHopGroups(model, out);
}

std::string_view CmdShowNextHopGroupsTraits::description() {
  return "Displays the switch's nexthop groups: each group's name and programming status, its member nexthops (address, egress interface, weight, cost), and any SRv6 SID list. Use it to inspect ECMP / nexthop-group programming.";
}

CmdShowNextHopGroups::RetType CmdShowNextHopGroups::sampleModel() {
  RetType model;

  // First group: unnamed, programmed, with two plain cost members
  cli::NextHopGroupEntry group1;
  group1.name() = "--";
  group1.isNamed() = false;
  group1.programmed() = "yes";
  group1.nextHops() = std::vector<std::string>{
      "fe80::200:11ff:fe22:3301 dev fboss2008 cost 202",
      "fe80::200:11ff:fe22:3302 dev fboss2002 cost 202"};
  model.nextHopGroups()->push_back(group1);

  // Second group: named "lspgrp_example", programmed, with an SRv6 SID list
  // member
  cli::NextHopGroupEntry group2;
  group2.name() = "lspgrp_example";
  group2.isNamed() = true;
  group2.programmed() = "yes";
  group2.nextHops() = std::vector<std::string>{
      "fe80::200:11ff:fe22:3301 dev fboss2008 SRv6 SID List [fdad:ffff:7fff::]"};
  model.nextHopGroups()->push_back(group2);

  return model;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowNextHopGroups, CmdShowNextHopGroupsTraits>::run();
template void
CmdHandler<CmdShowNamedNextHopGroups, CmdShowNamedNextHopGroupsTraits>::run();

} // namespace facebook::fboss
