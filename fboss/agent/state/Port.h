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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"

#include <boost/container/flat_map.hpp>
#include <string>

namespace facebook { namespace fboss {

class SwitchState;

struct PortFields {
  struct VlanInfo {
    explicit VlanInfo(bool emitTags) : tagged(emitTags) {}
    bool operator==(const VlanInfo& other) const {
      return tagged == other.tagged;
    }
    bool operator!=(const VlanInfo& other) const {
      return !(*this == other);
    }
    folly::dynamic toFollyDynamic() const;
    static VlanInfo fromFollyDynamic(const folly::dynamic& json);
    bool tagged;
  };
  typedef boost::container::flat_map<VlanID, VlanInfo> VlanMembership;

  PortFields(PortID id, std::string name)
    : id(id),
      name(name) {}

  template<typename Fn>
  void forEachChild(Fn fn) {}

  folly::dynamic toFollyDynamic() const;
  static PortFields fromFollyDynamic(const folly::dynamic& json);

  const PortID id{0};
  std::string name;
  std::string description;
  cfg::PortState state{cfg::PortState::DOWN}; // Administrative state
  bool operState{false}; // Operational state of port. UP(true), DOWN(false)
  VlanID ingressVlan{0};
  cfg::PortSpeed speed{cfg::PortSpeed::DEFAULT};
  cfg::PortSpeed maxSpeed{cfg::PortSpeed::DEFAULT};
  VlanMembership vlans;
};

/*
 * Port stores state about one of the physical ports on the switch.
 */
class Port : public NodeBaseT<Port, PortFields> {
 public:
  typedef PortFields::VlanInfo VlanInfo;
  typedef PortFields::VlanMembership VlanMembership;

  Port(PortID id, const std::string& name);

  static std::shared_ptr<Port>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = PortFields::fromFollyDynamic(json);
    return std::make_shared<Port>(fields);
  }

  static std::shared_ptr<Port>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  /*
   * Initialize a cfg::Port object with the default settings
   * that would be applied for this port. Port identifiers
   * like port ID and name are retained.
   *
   * The resulting cfg::Port object can be passed to applyConfig() to return
   * the port to a state as if it had been newly constructed.
   */
  void initDefaultConfigState(cfg::Port* config) const;

  PortID getID() const {
    return getFields()->id;
  }

  const std::string& getName() const {
    return getFields()->name;
  }

  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  const std::string& getDescription() const {
    return getFields()->description;
  }

  void setDescription(const std::string& description) {
    writableFields()->description = description;
  }

  cfg::PortState getState() const {
    return getFields()->state;
  }

  void setState(cfg::PortState state) {
    writableFields()->state = state;
  }

  bool getOperState() const {
    return getFields()->operState;
  }

  void setOperState(bool isUp) {
    writableFields()->operState = isUp;
  }

  bool isAdminDisabled() const {
    auto state = getFields()->state;
    return state == cfg::PortState::POWER_DOWN || state == cfg::PortState::DOWN;
  }

  /**
   * Tells you Oper+Admin state of port. Will be UP only if both admin and
   * oper state is UP.
   */
  bool isPortUp() const {
    auto const& fields = getFields();
    return fields->state == cfg::PortState::UP && fields->operState;
  }

  const VlanMembership& getVlans() const {
    return getFields()->vlans;
  }
  void setVlans(VlanMembership vlans) {
    writableFields()->vlans.swap(vlans);
  }

  VlanID getIngressVlan() const {
    return getFields()->ingressVlan;
  }
  void setIngressVlan(VlanID id) {
    writableFields()->ingressVlan = id;
  }

  cfg::PortSpeed getSpeed() const {
    return getFields()->speed;
  }
  void setSpeed(cfg::PortSpeed speed) {
    writableFields()->speed = speed;
  }

  cfg::PortSpeed getMaxSpeed() const {
    return getFields()->maxSpeed;
  }
  void setMaxSpeed(cfg::PortSpeed maxSpeed) {
    writableFields()->maxSpeed = maxSpeed;
  }

  cfg::PortSpeed getWorkingSpeed() const {
    if (getFields()->speed == cfg::PortSpeed::DEFAULT) {
      return getFields()->maxSpeed;
    } else {
      return getFields()->speed;
    }
  }

  Port* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
