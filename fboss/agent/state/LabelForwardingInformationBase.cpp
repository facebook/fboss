// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook {
namespace fboss {

LabelForwardingInformationBase::LabelForwardingInformationBase() {}

LabelForwardingInformationBase::~LabelForwardingInformationBase() {}

const std::shared_ptr<LabelForwardingEntry>&
LabelForwardingInformationBase::getLabelForwardingEntry(
    Label labelFib) const {
  return getNode(labelFib);
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::getLabelForwardingEntryIf(
    Label labelFib) const {
  return getNodeIf(labelFib);
}

std::shared_ptr<LabelForwardingInformationBase>
LabelForwardingInformationBase::fromFollyDynamic(const folly::dynamic& json) {
  auto labelFib = std::make_shared<LabelForwardingInformationBase>();
  if (json.isNull()) {
    return labelFib;
  }
  for (const auto& entry : json[BaseT::kEntries]) {
    labelFib->addNode(LabelForwardingEntry::fromFollyDynamic(entry));
  }
  return labelFib;
}

LabelForwardingInformationBase* LabelForwardingInformationBase::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto newFib = clone();
  auto* ptr = newFib.get();
  (*state)->resetLabelForwardingInformationBase(std::move(newFib));
  return ptr;
}

FBOSS_INSTANTIATE_NODE_MAP(
    LabelForwardingInformationBase,
    LabelForwardingRoute);

} // namespace fboss
} // namespace facebook
