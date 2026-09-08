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

#include <folly/Conv.h>

#include <boost/serialization/strong_typedef.hpp>
#include <fmt/format.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#define FBOSS_STRONG_TYPE(primitive, new_type)                              \
  namespace facebook::fboss {                                               \
                                                                            \
  BOOST_STRONG_TYPEDEF(primitive, new_type);                                \
                                                                            \
  /* Define toAppend() so folly::to<string> will work */                    \
  inline void toAppend(new_type value, std::string* result) {               \
    folly::toAppend(static_cast<primitive>(value), result);                 \
  }                                                                         \
  inline void toAppend(new_type value, folly::fbstring* result) {           \
    folly::toAppend(static_cast<primitive>(value), result);                 \
  }                                                                         \
                                                                            \
  } /* facebook::fboss */                                                   \
  namespace std {                                                           \
                                                                            \
  template <>                                                               \
  struct hash<facebook::fboss::new_type> {                                  \
    size_t operator()(const facebook::fboss::new_type& x) const {           \
      return hash<primitive>()(static_cast<primitive>(x));                  \
    }                                                                       \
  };                                                                        \
  } /* std */                                                               \
                                                                            \
  /* Support formatting with fmt::format as well */                         \
  namespace fmt {                                                           \
  template <>                                                               \
  struct formatter<facebook::fboss::new_type> {                             \
    template <typename ParseContext>                                        \
    constexpr auto parse(ParseContext& ctx) const {                         \
      return ctx.begin();                                                   \
    }                                                                       \
                                                                            \
    template <typename FormatContext>                                       \
    auto format(const facebook::fboss::new_type& value, FormatContext& ctx) \
        const {                                                             \
      return format_to(                                                     \
          ctx.out(), "{}({})", #new_type, static_cast<primitive>(value));   \
    }                                                                       \
  };                                                                        \
  } /* fmt */

/*
 * A unique ID identifying a node in the FBOSS state tree.
 */
FBOSS_STRONG_TYPE(uint64_t, NodeID)
