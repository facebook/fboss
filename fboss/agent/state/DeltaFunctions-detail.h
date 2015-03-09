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

/*
 * This file contains template helpers for implementing DeltaFunctions.
 */

namespace facebook { namespace fboss {
namespace DeltaFunctions { namespace detail {

/*
 * Helpers for invoking a user-supplied function and returning a LoopAction.
 *
 * If the user-supplied function returns a LoopAction, the return value is
 * passed through.  If the user-supplied function returns void,
 * LoopAction::CONTINUE is returned.
 */
template<typename Fn, typename... Args>
typename std::enable_if<std::is_same<typename std::result_of<Fn(Args...)>::type,
                                     LoopAction>::value &&
                          !std::is_member_function_pointer<Fn>::value,
                        LoopAction>::type
invokeFn(const Fn& fn, const Args&... args) {
  return fn(args...);
}

template<typename Fn, typename... Args>
typename std::enable_if<std::is_same<typename std::result_of<Fn(Args...)>::type,
                                     void>::value &&
                          !std::is_member_function_pointer<Fn>::value,
                        LoopAction>::type
invokeFn(const Fn& fn, const Args&... args) {
  fn(args...);
  return LoopAction::CONTINUE;
}

template<typename Fn, typename... Args>
typename std::enable_if<std::is_same<typename std::result_of<Fn(Args...)>::type,
                                     LoopAction>::value &&
                          std::is_member_function_pointer<Fn>::value,
                        LoopAction>::type
invokeFn(const Fn& fn, const Args&... args) {
  return std::mem_fn(fn)(args...);
}

template<typename Fn, typename... Args>
typename std::enable_if<std::is_same<typename std::result_of<Fn(Args...)>::type,
                                     void>::value &&
                          std::is_member_function_pointer<Fn>::value,
                        LoopAction>::type
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
template<typename ChangedFn, typename Node, typename... Args>
using IsActionChangedFn = std::is_same<typename std::result_of<
      ChangedFn(Args...,
                const std::shared_ptr<Node>&,
                const std::shared_ptr<Node>&)>::type,
      LoopAction>;
template<typename ChangedFn, typename Node, typename... Args>
using IsVoidChangedFn = std::is_same<typename std::result_of<
      ChangedFn(Args...,
                const std::shared_ptr<Node>&,
                const std::shared_ptr<Node>&)>::type,
      void>;
template<typename ChangedFn, typename Node, typename... Args>
using IsValidChangedFn = std::integral_constant<bool,
                         IsActionChangedFn<ChangedFn, Node, Args...>::value ||
                           IsVoidChangedFn<ChangedFn, Node, Args...>::value>;
template<typename AddRmFn, typename Node, typename... Args>
using IsActionAddRmFn = std::is_same<typename std::result_of<
      AddRmFn(Args..., const std::shared_ptr<Node>&)>::type,
      LoopAction>;
template<typename AddRmFn, typename Node, typename... Args>
using IsVoidAddRmFn = std::is_same<typename std::result_of<
      AddRmFn(Args..., const std::shared_ptr<Node>&)>::type,
      void>;
template<typename AddRmFn, typename Node, typename... Args>
using IsValidAddRmFn = std::integral_constant<bool,
                         IsActionAddRmFn<AddRmFn, Node, Args...>::value ||
                           IsVoidAddRmFn<AddRmFn, Node, Args...>::value>;

/*
 * Convenience wrappers around std::enable_if
 *
 * These evaluate to the LoopAction type, if the specified function arguments
 * are valid.
 */
template<typename ChangedFn, typename AddFn, typename RmFn, typename Node,
         typename... Args>
using EnableIfChangedAddRmFn =
  typename std::enable_if<IsValidChangedFn<ChangedFn, Node, Args...>::value &&
                          IsValidAddRmFn<AddFn, Node, Args...>::value &&
                          IsValidAddRmFn<RmFn, Node, Args...>::value,
                          LoopAction>::type;
template<typename ChangedFn, typename Node, typename... Args>
using EnableIfChangedFn =
  typename std::enable_if<IsValidChangedFn<ChangedFn, Node, Args...>::value,
                          LoopAction>::type;
template<typename AddRmFn, typename Node, typename... Args>
using EnableIfAddRmFn =
  typename std::enable_if<IsValidAddRmFn<AddRmFn, Node, Args...>::value,
                          LoopAction>::type;

}}
}} // facebook::fboss
