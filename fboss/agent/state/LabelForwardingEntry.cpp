// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/dynamic.h>

namespace facebook::fboss {

namespace {
auto constexpr kIncomingLabel = "topLabel";
auto constexpr kLabelNextHop = "labelNextHop";
auto constexpr kLabelNextHopsByClient = "labelNextHopMulti";
} // namespace

LabelForwardingEntry::LabelForwardingEntry(
    Label topLabel,
    ClientID clientId,
    LabelNextHopEntry nexthop)
    : NodeBaseT(topLabel, clientId, std::move(nexthop)) {
  updateLabelNextHop();
}

LabelForwardingEntry* LabelForwardingEntry::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  LabelForwardingInformationBase* labelFib =
      (*state)->getLabelForwardingInformationBase()->modify(state);
  auto newEntry = clone();
  auto* ptr = newEntry.get();
  labelFib->updateNode(std::move(newEntry));
  return ptr;
}

const LabelNextHopEntry* FOLLY_NULLABLE
LabelForwardingEntry::getEntryForClient(ClientID clientId) const {
  return getFields()->labelNextHopsByClient.getEntryForClient(clientId);
}

void LabelForwardingEntry::update(ClientID clientId, LabelNextHopEntry entry) {
  LabelForwardingEntryFields::validateLabelNextHopEntry(entry);
  writableFields()->labelNextHopsByClient.update(clientId, std::move(entry));
  updateLabelNextHop();
}

void LabelForwardingEntry::delEntryForClient(ClientID clientId) {
  writableFields()->labelNextHopsByClient.delEntryForClient(clientId);
  updateLabelNextHop();
}

void LabelForwardingEntry::updateLabelNextHop() {
  writableFields()->nexthop.reset();
  if (!getFields()->labelNextHopsByClient.isEmpty()) {
    auto bestEntry = getFields()->labelNextHopsByClient.getBestEntry();
    writableFields()->nexthop = *bestEntry.second;
  } else {
    writableFields()->nexthop = LabelNextHopEntry{
        LabelNextHopEntry::Action::DROP, AdminDistance::MAX_ADMIN_DISTANCE};
  }
}

folly::dynamic LabelForwardingEntry::toFollyDynamic() const {
  folly::dynamic json = folly::dynamic::object;
  json[kIncomingLabel] = static_cast<int>(getID());
  json[kLabelNextHop] = getLabelNextHop().toFollyDynamic();
  json[kLabelNextHopsByClient] = getLabelNextHopsByClient().toFollyDynamic();
  return json;
}

std::shared_ptr<LabelForwardingEntry> LabelForwardingEntry::fromFollyDynamic(
    const folly::dynamic& json) {
  LabelForwardingEntryFields fields;
  fields.topLabel = static_cast<MplsLabel>(json[kIncomingLabel].asInt());
  fields.nexthop = LabelNextHopEntry::fromFollyDynamic(json[kLabelNextHop]);
  fields.labelNextHopsByClient =
      LabelNextHopsByClient::fromFollyDynamic(json[kLabelNextHopsByClient]);

  return std::make_shared<LabelForwardingEntry>(fields);
}

bool LabelForwardingEntry::operator==(const LabelForwardingEntry& rhs) const {
  return getID() == rhs.getID() && getLabelNextHop() == rhs.getLabelNextHop() &&
      getLabelNextHopsByClient() == rhs.getLabelNextHopsByClient();
}

void LabelForwardingEntryFields::validateLabelNextHopEntry(
    const LabelNextHopEntry& entry) {
  if (!LabelForwardingInformationBase::isValidNextHopSet(
          entry.getNextHopSet())) {
    throw FbossError("invalid label forwarding entry");
  }
}

std::string LabelForwardingEntry::str() const {
  return folly::to<std::string>(
      getID(),
      "@",
      getLabelNextHopsByClient().str(),
      ", =>",
      getLabelNextHop().str());
}
template class NodeBaseT<LabelForwardingEntry, LabelForwardingEntryFields>;

} // namespace facebook::fboss
