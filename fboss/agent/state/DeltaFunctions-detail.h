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

#include <functional>

/*
 * This file contains template helpers for implementing DeltaFunctions.
 */

namespace facebook::fboss {

namespace DeltaFunctions {
namespace detail {

/*
 * Helpers for invoking a user-supplied function and returning a LoopAction.
 *
 * If the user-supplied function returns a LoopAction, the return value is
 * passed through.  If the user-supplied function returns void,
 * LoopAction::CONTINUE is returned.
 */
template <typename Fn, typename... Args>
std::enable_if_t<
    std::is_same<std::result_of_t<Fn(Args...)>, LoopAction>::value &&
        !std::is_member_function_pointer<Fn>::value,
    LoopAction>
invokeFn(const Fn& fn, const Args&... args) {
  return fn(args...);
}

template <typename Fn, typename... Args>
std::enable_if_t<
    std::is_same<std::result_of_t<Fn(Args...)>, void>::value &&
        !std::is_member_function_pointer<Fn>::value,
    LoopAction>
invokeFn(const Fn& fn, const Args&... args) {
  fn(args...);
  return LoopAction::CONTINUE;
}

template <typename Fn, typename... Args>
std::enable_if_t<
    std::is_same<std::result_of_t<Fn(Args...)>, LoopAction>::value &&
        std::is_member_function_pointer<Fn>::value,
    LoopAction>
invokeFn(const Fn& fn, const Args&... args) {
  return std::mem_fn(fn)(args...);
}

template <typename Fn, typename... Args>
std::enable_if_t<
    std::is_same<std::result_of_t<Fn(Args...)>, void>::value &&
        std::is_member_function_pointer<Fn>::value,
    LoopAction>
invokeFn(const Fn& fn, const Args&... args) {
  std::mem_fn(fn)(args...);
  return LoopAction::CONTINUE;
}

/*
 * Templates to check if a function is valid for passing to
 * forEachChanged(), forEachAdded(), or forEachRemoved()
 *
 * These are used with std::enable_if<> below
 */
template <typename ChangedFn, typename Node, typename... Args>
using IsActionChangedFn = std::is_same<
    std::result_of_t<ChangedFn(
        Args...,
        const std::shared_ptr<Node>&,
        const std::shared_ptr<Node>&)>,
    LoopAction>;
template <typename ChangedFn, typename Node, typename... Args>
using IsVoidChangedFn = std::is_same<
    std::result_of_t<ChangedFn(
        Args...,
        const std::shared_ptr<Node>&,
        const std::shared_ptr<Node>&)>,
    void>;
template <typename ChangedFn, typename Node, typename... Args>
using IsValidChangedFn = std::integral_constant<
    bool,
    IsActionChangedFn<ChangedFn, Node, Args...>::value ||
        IsVoidChangedFn<ChangedFn, Node, Args...>::value>;
template <typename AddRmFn, typename Node, typename... Args>
using IsActionAddRmFn = std::is_same<
    std::result_of_t<AddRmFn(Args..., const std::shared_ptr<Node>&)>,
    LoopAction>;
template <typename AddRmFn, typename Node, typename... Args>
using IsVoidAddRmFn = std::is_same<
    std::result_of_t<AddRmFn(Args..., const std::shared_ptr<Node>&)>,
    void>;
template <typename AddRmFn, typename Node, typename... Args>
using IsValidAddRmFn = std::integral_constant<
    bool,
    IsActionAddRmFn<AddRmFn, Node, Args...>::value ||
        IsVoidAddRmFn<AddRmFn, Node, Args...>::value>;

/*
 * Convenience wrappers around std::enable_if
 *
 * These evaluate to the LoopAction type, if the specified function arguments
 * are valid.
 */
template <
    typename ChangedFn,
    typename AddFn,
    typename RmFn,
    typename Node,
    typename... Args>
using EnableIfChangedAddRmFn = std::enable_if_t<
    IsValidChangedFn<ChangedFn, Node, Args...>::value &&
        IsValidAddRmFn<AddFn, Node, Args...>::value &&
        IsValidAddRmFn<RmFn, Node, Args...>::value,
    LoopAction>;
template <typename ChangedFn, typename Node, typename... Args>
using EnableIfChangedFn = std::
    enable_if_t<IsValidChangedFn<ChangedFn, Node, Args...>::value, LoopAction>;
template <typename AddRmFn, typename Node, typename... Args>
using EnableIfAddRmFn =
    std::enable_if_t<IsValidAddRmFn<AddRmFn, Node, Args...>::value, LoopAction>;

} // namespace detail
} // namespace DeltaFunctions

} // namespace facebook::fboss
