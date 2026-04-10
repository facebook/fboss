// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/mysid/CmdShowMySid.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowMySid::RetType CmdShowMySid::queryClient(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  std::vector<MySidEntry> entries;
  client->sync_getMySidEntries(entries);
  return createModel(entries);
}

CmdShowMySid::RetType CmdShowMySid::createModel(
    const std::vector<MySidEntry>& entries) {
  RetType model;
  for (const auto& entry : entries) {
    cli::MySidEntryModel entryModel;

    // Format prefix as addr/len
    const auto& mySid = entry.mySid().value();
    auto prefixAddr = network::toIPAddress(mySid.prefixAddress().value()).str();
    entryModel.prefix() =
        prefixAddr + "/" + std::to_string(mySid.prefixLength().value());

    entryModel.type() =
        apache::thrift::util::enumNameSafe(entry.type().value());

    for (const auto& nh : entry.nextHops().value()) {
      entryModel.nextHops()->push_back(
          network::toIPAddress(nh.address().value()).str());
    }
    for (const auto& nh : entry.resolvedNextHops().value()) {
      entryModel.resolvedNextHops()->push_back(
          network::toIPAddress(nh.address().value()).str());
    }

    model.mySidEntries()->push_back(std::move(entryModel));
  }
  return model;
}

void CmdShowMySid::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.mySidEntries().value()) {
    out << fmt::format(
        "MySid: {}  Type: {}\n", entry.prefix().value(), entry.type().value());
    for (const auto& nh : entry.nextHops().value()) {
      out << fmt::format("\tunresolved next hops {}\n", nh);
    }
    for (const auto& nh : entry.resolvedNextHops().value()) {
      out << fmt::format("\tresolved via {}\n", nh);
    }
  }
}

// Explicit template instantiation
template void CmdHandler<CmdShowMySid, CmdShowMySidTraits>::run();

} // namespace facebook::fboss
