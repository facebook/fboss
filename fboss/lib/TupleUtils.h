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

#include <optional>
#include <tuple>
#include <type_traits>

namespace facebook::fboss {
/*
 * Regular functions
 *
 * This part of the library operates on runtime tuples and perform some
 * transformation or operation like a loop, mapping, projection, etc...
 *
 * e.g., for each element in a tuple, call foo() on it
 */

template <typename Fn, typename TupleT>
void tupleForEach(const Fn& fn, TupleT& tup) {
  std::apply([&](auto&... t) { (..., fn(t)); }, tup);
}

template <typename Fn, typename TupleT>
auto tupleMap(const Fn& fn, TupleT&& tup) {
  auto map_helper = [&fn](auto&&... attr) { return std::tuple{fn(attr)...}; };
  return std::apply(map_helper, tup);
}

template <typename FullTupleT, typename ProjectionTupleT>
ProjectionTupleT tupleProjection(const FullTupleT& fullTuple) {
  ProjectionTupleT ret;
  auto projection_helper = [&ret, &fullTuple](auto& t) {
    std::get<typename std::remove_reference<decltype(t)>::type>(ret) =
        std::get<typename std::remove_reference<decltype(t)>::type>(fullTuple);
  };
  tupleForEach(projection_helper, ret);
  return ret;
}

/*
 * Meta Functions
 *
 * This part of the library operates on tuple types to modify them in some way
 * For example, concatenation or filtering based on some other meta function.
 *
 * e.g., given two tuple types, get the type of the tuple of all the elements of
 * both. Or, given a tuple type and a meta function, get the type of the tuple
 * whose element types have a true value under that meta function
 */

template <typename T1, typename T2>
struct IsElementOfTuple : std::false_type {};

template <typename T, typename... Ts>
struct IsElementOfTuple<T, std::tuple<Ts...>>
    : std::disjunction<std::is_same<T, Ts>...> {};

template <typename T1, typename T2>
struct IsSubsetOfTuple : std::false_type {};

template <typename... T1s, typename... T2s>
struct IsSubsetOfTuple<std::tuple<T1s...>, std::tuple<T2s...>>
    : std::conjunction<IsElementOfTuple<T1s, std::tuple<T2s...>>...> {};

template <typename T1, typename T2>
struct tupleConcat {};
template <typename... T1s, typename... T2s>
struct tupleConcat<std::tuple<T1s...>, std::tuple<T2s...>> {
  using type = std::tuple<T1s..., T2s...>;
};
template <typename T1, typename T2>
using tupleConcat_t = typename tupleConcat<T1, T2>::type;

template <typename T>
struct makeTupleElementsOptional {};
template <typename... Ts>
struct makeTupleElementsOptional<std::tuple<Ts...>> {
  using type = std::tuple<std::optional<Ts>...>;
};
template <typename T>
using makeTupleElementsOptional_t = typename makeTupleElementsOptional<T>::type;

template <typename>
struct IsTuple : std::false_type {};

template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {};

template <typename, typename>
struct TupleIndex {};

template <typename ElementType, typename... Types>
struct TupleIndex<std::tuple<Types...>, ElementType> {
 private:
  static constexpr auto Size = sizeof...(Types);
  using Tuple = std::tuple<Types...>;

  template <std::size_t Index = 0>
  static constexpr std::size_t index() {
    static_assert(Index < Size, "ElementType not in Tuple");
    if constexpr (std::is_same_v<
                      ElementType,
                      std::tuple_element_t<Index, Tuple>>) {
      return Index;
    } else {
      return index<Index + 1>();
    }
  }

 public:
  static auto constexpr value = index();
};
} // namespace facebook::fboss
