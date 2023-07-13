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

#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::thrift_cow {

namespace detail {

template <typename TC>
constexpr bool tc_is_struct_or_union =
    std::is_same_v<TC, apache::thrift::type_class::structure> ||
    std::is_same_v<TC, apache::thrift::type_class::variant>;

}

template <fsdb::OperProtocol Protocol>
struct ProtocolSerializers {
  static_assert(Protocol != Protocol, "Unsupported Protocol");
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::BINARY> {
  using Reader = apache::thrift::BinaryProtocolReader;
  using Writer = apache::thrift::BinaryProtocolWriter;
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::SIMPLE_JSON> {
  using Reader = apache::thrift::SimpleJSONProtocolReader;
  using Writer = apache::thrift::SimpleJSONProtocolWriter;
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::COMPACT> {
  using Reader = apache::thrift::CompactProtocolReader;
  using Writer = apache::thrift::CompactProtocolWriter;
};

template <fsdb::OperProtocol Protocol>
struct Serializer {
  using Serializers = ProtocolSerializers<Protocol>;
  using Reader = typename Serializers::Reader;
  using Writer = typename Serializers::Writer;
  using TSerializer = apache::thrift::Serializer<Reader, Writer>;

  template <
      typename TC,
      typename TType,
      std::enable_if_t<detail::tc_is_struct_or_union<TC>, bool> = true>
  static folly::fbstring serialize(const TType& ttype) {
    folly::fbstring encoded;
    TSerializer::serialize(ttype, &encoded);
    encoded.shrink_to_fit();
    return encoded;
  }

  template <
      typename TC,
      typename TType,
      std::enable_if_t<!detail::tc_is_struct_or_union<TC>, bool> = true>
  static folly::fbstring serialize(const TType& ttype) {
    folly::IOBufQueue queue;
    Writer writer;
    writer.setOutput(&queue);
    apache::thrift::detail::pm::protocol_methods<TC, TType>::write(
        writer, ttype);
    auto str = queue.move()->moveToFbString();
    str.shrink_to_fit();
    return str;
  }

  template <
      typename TC,
      typename TType,
      std::enable_if_t<detail::tc_is_struct_or_union<TC>, bool> = true>
  static TType deserialize(const folly::fbstring& encoded) {
    return TSerializer::template deserialize<TType>(encoded.toStdString());
  }

  template <
      typename TC,
      typename TType,
      std::enable_if_t<!detail::tc_is_struct_or_union<TC>, bool> = true>
  static TType deserialize(const folly::fbstring& encoded) {
    Reader reader;
    auto buf = folly::IOBuf::copyBuffer(encoded.data(), encoded.length());
    reader.setInput(buf.get());
    TType recovered;
    apache::thrift::detail::pm::protocol_methods<TC, TType>::read(
        reader, recovered);
    return recovered;
  }
};

template <typename TC, typename TType>
folly::fbstring serialize(fsdb::OperProtocol proto, const TType& ttype) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<fsdb::OperProtocol::BINARY>::template serialize<TC>(
          ttype);
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::template serialize<
          TC>(ttype);
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::template serialize<TC>(
          ttype);
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

template <typename TC, typename TType>
TType deserialize(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<
          fsdb::OperProtocol::BINARY>::template deserialize<TC, TType>(encoded);
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::
          template deserialize<TC, TType>(encoded);
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::
          template deserialize<TC, TType>(encoded);
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

} // namespace facebook::fboss::thrift_cow
