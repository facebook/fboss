/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

struct RouteTableFields {
  explicit RouteTableFields(RouterID id);
  template <typename Fn>
  void forEachChild(Fn fn) {
    fn(ribV4.get());
    fn(ribV6.get());
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Deserialize from folly::dynamic
   */
  static RouteTableFields fromFollyDynamic(const folly::dynamic& json);

  const RouterID id{0};
  typedef RouteTableRib<folly::IPAddressV4> RibTypeV4;
  typedef RouteTableRib<folly::IPAddressV6> RibTypeV6;
  std::shared_ptr<RibTypeV4> ribV4;
  std::shared_ptr<RibTypeV6> ribV6;
};

class RouteTable : public NodeBaseT<RouteTable, RouteTableFields> {
 public:
  typedef RouteTableFields::RibTypeV4 RibTypeV4;
  typedef RouteTableFields::RibTypeV6 RibTypeV6;

  explicit RouteTable(RouterID id);
  ~RouteTable() override;

  static std::shared_ptr<RouteTable> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = RouteTableFields::fromFollyDynamic(json);
    return std::make_shared<RouteTable>(fields);
  }

  static std::shared_ptr<RouteTable> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toFollyDynamic();
  }

  RouterID getID() const {
    return getFields()->id;
  }
  const std::shared_ptr<RibTypeV4>& getRibV4() const {
    return getFields()->ribV4;
  }
  const std::shared_ptr<RibTypeV6>& getRibV6() const {
    return getFields()->ribV6;
  }
  template <typename AddressT>
  const std::shared_ptr<RouteTableRib<AddressT>> getRib() const;

  bool empty() const;

  RouteTable* modify(std::shared_ptr<SwitchState>* state);

  std::shared_ptr<RibTypeV4>& writableRibV4() {
    return writableFields()->ribV4;
  }
  std::shared_ptr<RibTypeV6>& writableRibV6() {
    return writableFields()->ribV6;
  }

  void setRib(std::shared_ptr<RibTypeV4> rib) {
    writableFields()->ribV4.swap(rib);
  }
  void setRib(std::shared_ptr<RibTypeV6> rib) {
    writableFields()->ribV6.swap(rib);
  }

 private:
  // Forbidden copy constructor and assignment operator
  RouteTable(RouteTable const&) = delete;
  RouteTable& operator=(RouteTable const&) = delete;

  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

template <>
inline const std::shared_ptr<RouteTableRib<folly::IPAddressV4>>
RouteTable::getRib() const {
  return getRibV4();
}

template <>
inline const std::shared_ptr<RouteTableRib<folly::IPAddressV6>>
RouteTable::getRib() const {
  return getRibV6();
}

} // namespace facebook::fboss
