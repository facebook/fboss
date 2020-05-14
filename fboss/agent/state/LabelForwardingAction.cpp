// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingAction.h"

namespace facebook::fboss {

namespace {
constexpr auto kType = "type";
constexpr auto kSwapWith = "swapWith";
constexpr auto kPushStack = "pushStack";
} // namespace

folly::dynamic LabelForwardingAction::toFollyDynamic() const {
  folly::dynamic labelForwardingAction = folly::dynamic::object;
  labelForwardingAction[kType] = static_cast<int>(type_);
  if (swapWith_) {
    labelForwardingAction[kSwapWith] = static_cast<int>(swapWith_.value());
  }
  if (pushStack_) {
    labelForwardingAction[kPushStack] = folly::dynamic::array();
    for (const auto& label : pushStack_.value()) {
      labelForwardingAction[kPushStack].push_back(static_cast<int>(label));
    }
  }
  return labelForwardingAction;
}

MplsAction LabelForwardingAction::toThrift() const {
  MplsAction mplsActionThrift;
  *mplsActionThrift.action_ref() = type_;
  if (swapWith_) {
    mplsActionThrift.swapLabel_ref() = swapWith_.value();
  } else if (pushStack_) {
    MplsLabelStack pushStack;
    for (const auto& label : pushStack_.value()) {
      pushStack.push_back(label);
    }
    auto thriftPushStack = mplsActionThrift.pushLabels_ref();
    thriftPushStack = std::move(pushStack);
  }
  return mplsActionThrift;
}

LabelForwardingAction LabelForwardingAction::fromThrift(
    const MplsAction& mplsAction) {
  if (mplsAction.swapLabel_ref()) {
    return LabelForwardingAction(
        *mplsAction.action_ref(), mplsAction.swapLabel_ref().value());
  } else if (mplsAction.pushLabels_ref()) {
    return LabelForwardingAction(
        *mplsAction.action_ref(), mplsAction.pushLabels_ref().value());
  }
  return LabelForwardingAction(*mplsAction.action_ref());
}

LabelForwardingAction LabelForwardingAction::fromFollyDynamic(
    const folly::dynamic& json) {
  LabelForwardingType type =
      static_cast<LabelForwardingType>(json[kType].asInt());
  if (json.find(kSwapWith) != json.items().end()) {
    return LabelForwardingAction(type, json[kSwapWith].asInt());
  } else if (json.find(kPushStack) != json.items().end()) {
    LabelStack stack;
    for (const auto& label : json[kPushStack]) {
      stack.push_back(static_cast<Label>(label.asInt()));
    }
    return LabelForwardingAction(type, stack);
  }
  return LabelForwardingAction(type);
}

bool LabelForwardingAction::operator==(const LabelForwardingAction& rhs) const {
  return type() == rhs.type() && swapWith() == rhs.swapWith() &&
      pushStack() == rhs.pushStack();
}

bool LabelForwardingAction::operator<(const LabelForwardingAction& rhs) const {
  return std::tie(type_, swapWith_, pushStack_) <
      std::tie(rhs.type_, rhs.swapWith_, rhs.pushStack_);
}

std::optional<LabelForwardingAction>
LabelForwardingAction::combinePushLabelStack(
    const std::optional<LabelForwardingAction>& tunnelStack,
    const std::optional<LabelForwardingAction>& adjacencyLabels) {
  // create a label stack based on tunnel stack and adjacency labels
  // a tunnel stack - a stack of labels advertised by remote label distribution
  //    peer Y, for prefix X
  // an adjacency labels - a stack of labels advertised by adjacent (or lesser
  //    remote) label distribution peer to network of Y
  //  Push adjacency labels on top of tunnel stack, egressing IP2MPLS packets
  //  will have adjacency labels on top of stack
  std::optional<LabelForwardingAction> result = std::nullopt;
  if (!tunnelStack && !adjacencyLabels) {
    return result;
  }
  LabelForwardingAction::LabelStack stack;
  if (!tunnelStack) {
    CHECK(
        LabelForwardingAction::LabelForwardingType::PUSH ==
        adjacencyLabels->type());
    return adjacencyLabels;
  } else if (!adjacencyLabels) {
    CHECK(
        LabelForwardingAction::LabelForwardingType::PUSH ==
        tunnelStack->type());
    return tunnelStack;
  } else {
    CHECK(
        LabelForwardingAction::LabelForwardingType::PUSH ==
        tunnelStack->type());
    CHECK(
        LabelForwardingAction::LabelForwardingType::PUSH ==
        adjacencyLabels->type());

    const auto& leftStack = tunnelStack->pushStack().value();
    stack.insert(stack.end(), leftStack.begin(), leftStack.end());
    const auto& rightStack = adjacencyLabels->pushStack().value();
    stack.insert(stack.end(), rightStack.begin(), rightStack.end());
  }
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack));
}

} // namespace facebook::fboss
