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

#include <cstdint>
#include <memory>
#include <type_traits>

namespace facebook::fboss {

/*
 * A return type so that user-supplied functions can specify if they want to
 * continue to be called with more changed nodes, or if they want to stop.
 */
enum class LoopAction : uint32_t {
  BREAK,
  CONTINUE,
};

struct DeltaComparison {
  enum class Policy : uint32_t {
    SHALLOW,
    DEEP,
  };
  struct PolicyRAII {
    // thread unsafe in favor of avoiding synchronization and better run time,
    // must be invoked in context of single state delta processing thread
    explicit PolicyRAII(const Policy& policy)
        : previousComparison_(comparison_) {
      comparison_ = policy;
    }
    ~PolicyRAII() {
      comparison_ = previousComparison_;
    }

   private:
    const Policy previousComparison_;
  };
  static inline Policy policy() {
    return comparison_;
  }

 private:
  friend PolicyRAII;
  static inline Policy comparison_{Policy::SHALLOW};
};

} // namespace facebook::fboss

#include "fboss/agent/state/DeltaFunctions-detail.h"

/*
 * The DeltaFunctions namespace contains utility functions for invoking
 * caller-supplied functions on each change in a NodeMapDelta.
 *
 * Notably, these are the forEachChanged(), forEachAdded(), and
 * forEachRemoved() functions.
 */
namespace facebook::fboss::DeltaFunctions {

/*
 * Invoke the specified functions for each modified, added, and removed node.
 */
template <
    typename Delta,
    typename ChangedFn,
    typename AddFn,
    typename RemoveFn,
    typename... Args>
detail::EnableIfChangedAddRmFn<
    ChangedFn,
    AddFn,
    RemoveFn,
    typename Delta::NodeWrapper,
    Args...>
forEachChanged(
    const Delta& delta,
    ChangedFn changedFn,
    AddFn addedFn,
    RemoveFn removedFn,
    const Args&... args) {
  for (const auto& entry : delta) {
    const auto& oldNode = entry.getOld();
    const auto& newNode = entry.getNew();
    LoopAction action;
    if (oldNode) {
      if (newNode) {
        if (DeltaComparison::policy() == DeltaComparison::Policy::DEEP) {
          if (*oldNode == *newNode) {
            // when delta comparison policy is deep, compare contents
            // this is an expensive operation, and must be used carefully
            continue;
          }
        }
        action = detail::invokeFn(changedFn, args..., oldNode, newNode);
      } else {
        action = detail::invokeFn(removedFn, args..., oldNode);
      }
    } else {
      action = detail::invokeFn(addedFn, args..., newNode);
    }
    if (action == LoopAction::BREAK) {
      return action;
    }
  }
  return LoopAction::CONTINUE;
}

/*
 * Invoke the specified function for each modified node.
 */
template <typename Delta, typename ChangedFn, typename... Args>
detail::EnableIfChangedFn<ChangedFn, typename Delta::NodeWrapper, Args...>
forEachChanged(const Delta& delta, ChangedFn changedFn, const Args&... args) {
  for (const auto& entry : delta) {
    const auto& oldNode = entry.getOld();
    const auto& newNode = entry.getNew();
    if (oldNode && newNode) {
      if (DeltaComparison::policy() == DeltaComparison::Policy::DEEP) {
        if (*oldNode == *newNode) {
          // when delta comparison policy is deep, compare contents
          // this is an expensive operation, and must be used carefully
          continue;
        }
      }
      LoopAction action =
          detail::invokeFn(changedFn, args..., oldNode, newNode);
      if (action == LoopAction::BREAK) {
        return action;
      }
    }
  }
  return LoopAction::CONTINUE;
}

/*
 * Invoke the specified function for each added node.
 */
template <typename Delta, typename AddedFn, typename... Args>
detail::EnableIfAddRmFn<AddedFn, typename Delta::NodeWrapper, Args...>
forEachAdded(const Delta& delta, AddedFn addedFn, const Args&... args) {
  for (const auto& entry : delta) {
    if (!entry.getOld()) {
      LoopAction action = detail::invokeFn(addedFn, args..., entry.getNew());
      if (action == LoopAction::BREAK) {
        return action;
      }
    }
  }
  return LoopAction::CONTINUE;
}

/*
 * Invoke the specified function for each removed node.
 */
template <typename Delta, typename RemovedFn, typename... Args>
detail::EnableIfAddRmFn<RemovedFn, typename Delta::NodeWrapper, Args...>
forEachRemoved(const Delta& delta, RemovedFn removedFn, const Args&... args) {
  for (const auto& entry : delta) {
    if (!entry.getNew()) {
      LoopAction action = detail::invokeFn(removedFn, args..., entry.getOld());
      if (action == LoopAction::BREAK) {
        return action;
      }
    }
  }
  return LoopAction::CONTINUE;
}

/*
 * Delta is empty
 */
template <typename Delta>
bool isEmpty(const Delta& delta) {
  using NodeWrapper = typename Delta::NodeWrapper;
  bool empty = true;
  auto hasChanged = [&empty](const NodeWrapper&, const NodeWrapper&) {
    empty = false;
    return LoopAction::BREAK;
  };
  auto hasAdded = [&empty](const NodeWrapper&) {
    empty = false;
    return LoopAction::BREAK;
  };
  auto hasRemoved = hasAdded;
  forEachChanged(delta, hasChanged, hasAdded, hasRemoved);
  return empty;
}

} // namespace facebook::fboss::DeltaFunctions
