// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/dynamic.h>
#include <optional>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace {
auto constexpr kInvalidAction = "Invalid Label Forwarding Action";
} // namespace

namespace facebook::fboss {

/* LabelForwardingAction defines an MPLS forwarding action and parameters
 * required necessary for that action. Label forwarding action is associated
 * with a particular NextHop.
 *
 * For IP2MPLS route:
 * LabelForwardingAction is implied to be PUSH with pushStack_ as required label
 * stack.
 * For MPLS2MPLS route:
 * LabelForwardingAction will be either PUSH/SWAP/PHP/POP/NOOP with swapWith_
 * used only if action is SWAP.
 *
 * This object encapsulates required state information to program MPLS egress to
 * next hop in underlying H/W.
 *
 * This object will be referenced from NextHop are which are held in
 *  - RoutingInformationBase (for IP2MPLS routes)
 *  - IncomingLabelMap (for MPLS2MPLS routes)
 */
class LabelForwardingAction {
 public:
  using LabelForwardingType = MplsActionCode;
  using Label = MplsLabel;
  using LabelStack = MplsLabelStack;

  LabelForwardingAction(LabelForwardingType type, Label swapWith)
      : type_(type), swapWith_(swapWith) {
    if (type_ != LabelForwardingType::SWAP) {
      throw FbossError(folly::to<std::string>(
          kInvalidAction,
          ": label to swap with is not valid for label forwarding type ",
          type_));
    }
  }

  LabelForwardingAction(LabelForwardingType type, LabelStack pushStack)
      : type_(type), pushStack_(std::move(pushStack)) {
    if (type_ != LabelForwardingType::PUSH || !pushStack_ ||
        !pushStack_->size()) {
      throw FbossError(folly::to<std::string>(
          kInvalidAction,
          ": either label stack to push is missing or ",
          "label stack is provided for label forwarding type ",
          type_));
    }
  }

  explicit LabelForwardingAction(LabelForwardingType type) : type_(type) {
    if (type_ != LabelForwardingType::POP_AND_LOOKUP &&
        type_ != LabelForwardingType::PHP &&
        type_ != LabelForwardingType::NOOP) {
      throw FbossError(folly::to<std::string>(
          kInvalidAction,
          ": required attributes missing for label forwarding type ",
          type_));
    }
  }

  const std::optional<Label>& swapWith() const {
    return swapWith_;
  }

  const std::optional<LabelStack>& pushStack() const {
    return pushStack_;
  }

  LabelForwardingType type() const {
    return type_;
  }

  folly::dynamic toFollyDynamic() const;

  MplsAction toThrift() const;

  static LabelForwardingAction fromThrift(const MplsAction& mplsAction);

  static LabelForwardingAction fromFollyDynamic(const folly::dynamic& json);

  bool operator==(const LabelForwardingAction& rhs) const;

  bool operator!=(const LabelForwardingAction& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const LabelForwardingAction& rhs) const;

  static std::optional<LabelForwardingAction> combinePushLabelStack(
      const std::optional<LabelForwardingAction>& tunnelStack,
      const std::optional<LabelForwardingAction>& adjacencyLabels);

 private:
  LabelForwardingType type_{LabelForwardingType::NOOP};
  std::optional<Label> swapWith_;
  std::optional<LabelStack> pushStack_;
};

} // namespace facebook::fboss
