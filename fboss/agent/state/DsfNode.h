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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct DsfNodeFields : public ThriftyFields<DsfNodeFields, cfg::DsfNode> {
  explicit DsfNodeFields(SwitchID id) {
    auto& data = writableData();
    *data.switchId() = id;
  }

  bool operator==(const DsfNodeFields& other) const {
    return data() == other.data();
  }

  cfg::DsfNode toThrift() const override {
    return data();
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  static DsfNodeFields fromThrift(const cfg::DsfNode& systemPortThrift);
};

class DsfNode : public ThriftyBaseT<cfg::DsfNode, DsfNode, DsfNodeFields> {
 public:
  SwitchID getID() const {
    return SwitchID(*getFields()->data().switchId());
  }
  SwitchID getSwitchId() const {
    return SwitchID(*getFields()->data().switchId());
  }
  std::string getName() const {
    return *getFields()->data().name();
  }
  void setName(const std::string& name) {
    writableFields()->writableData().name() = name;
  }
  cfg::DsfNodeType getType() const {
    return *getFields()->data().type();
  }

  const std::vector<std::string>& getLoopbackIps() const {
    return *getFields()->data().loopbackIps();
  }
  void setLoopbackIps(const std::vector<std::string>& loopbackIps) {
    *writableFields()->writableData().loopbackIps() = loopbackIps;
  }
  const cfg::Range64& getSystemPortRange() const {
    return *getFields()->data().systemPortRange();
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<cfg::DsfNode, DsfNode, DsfNodeFields>::ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
