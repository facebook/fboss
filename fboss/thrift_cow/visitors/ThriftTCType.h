// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/TypeClass.h>

namespace facebook::fboss::thrift_cow {

// Simplified enum representing main thrift type classes we care about We
// convert to this in template params whenever possible to reduce total number
// of types generated. Thrift TC containers (map, list, set) are templated on
// key and val types so they will generate more types if used
enum class ThriftTCType {
  PRIMITIVE,
  STRUCTURE,
  VARIANT,
  MAP,
  SET,
  LIST,
};

template <typename TC>
struct TCToTCType;

template <>
struct TCToTCType<apache::thrift::type_class::structure> {
  static constexpr auto type = ThriftTCType::STRUCTURE;
};

template <>
struct TCToTCType<apache::thrift::type_class::variant> {
  static constexpr auto type = ThriftTCType::VARIANT;
};

template <typename ValTC>
struct TCToTCType<apache::thrift::type_class::set<ValTC>> {
  static constexpr auto type = ThriftTCType::SET;
};

template <typename KeyTC, typename ValTC>
struct TCToTCType<apache::thrift::type_class::map<KeyTC, ValTC>> {
  static constexpr auto type = ThriftTCType::MAP;
};

template <typename ValTC>
struct TCToTCType<apache::thrift::type_class::list<ValTC>> {
  static constexpr auto type = ThriftTCType::LIST;
};

template <typename TC>
struct TCToTCType {
  static constexpr auto type = ThriftTCType::PRIMITIVE;
};

template <typename TC>
static constexpr auto TCType = TCToTCType<TC>::type;

} // namespace facebook::fboss::thrift_cow
