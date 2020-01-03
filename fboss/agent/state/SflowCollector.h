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

#include <string>
#include <utility>

#include <folly/SocketAddress.h>

#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct SflowCollectorFields {
  SflowCollectorFields(const std::string& ip, const uint16_t port)
      : address(ip, port),
        id(address.getFullyQualified() + ':' +
           folly::to<std::string>(address.getPort())) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static SflowCollectorFields fromFollyDynamic(const folly::dynamic& json);

  const folly::SocketAddress address;
  const std::string id;
};

/*
 * SflowCollector stores the IP and port of a UDP-based collector of sFlow
 * samples.
 */
class SflowCollector : public NodeBaseT<SflowCollector, SflowCollectorFields> {
 public:
  SflowCollector(const std::string& ip, const uint16_t port);

  static std::shared_ptr<SflowCollector> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = SflowCollectorFields::fromFollyDynamic(json);
    return std::make_shared<SflowCollector>(fields);
  }

  static std::shared_ptr<SflowCollector> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  const std::string& getID() const {
    return getFields()->id;
  }

  const folly::SocketAddress& getAddress() const {
    return getFields()->address;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
