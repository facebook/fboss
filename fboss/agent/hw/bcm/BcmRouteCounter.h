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

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include <folly/json/dynamic.h>
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/lib/RefMap.h"

#define STAT_MODEID_INVALID -1

namespace facebook::fboss {

/*
 * This object represents the BCM route counter object.
 * On a tomahawk4 device, it contains the flex action id
 * and offset which are both attached to the route.
 * On a tomahawk3 device, only the id is used. offset is 0.
 */
class BcmRouteCounterID {
 public:
  BcmRouteCounterID() : id_(0), offset_(0) {}
  BcmRouteCounterID(uint32_t id, uint32_t offset) : id_(id), offset_(offset) {}
  ~BcmRouteCounterID() {}
  folly::dynamic toFollyDynamic() const;
  static BcmRouteCounterID fromFollyDynamic(const folly::dynamic& json);
  uint32_t getHwId() const {
    return id_;
  }
  uint32_t getHwOffset() const {
    return offset_;
  }
  std::string str() const {
    return (fmt::format("id={};ofset={}", id_, offset_));
  }

  bool operator==(const BcmRouteCounterID& that) const {
    return (id_ == that.id_ && offset_ == that.offset_);
  }

  bool operator!=(const BcmRouteCounterID& that) const {
    return !(*this == that);
  }

  bool operator<(const BcmRouteCounterID& that) const {
    return (std::tie(id_, offset_) < std::tie(that.id_, that.offset_));
  }
  static constexpr folly::StringPiece kCounterIndex{"index"};
  static constexpr folly::StringPiece kCounterOffset{"offset"};

 private:
  uint32_t id_;
  uint32_t offset_;
};

/**
 * BcmRouteCounter represents counter associated with L3 route object.
 */
class BcmRouteCounterBase {
 public:
  BcmRouteCounterBase(BcmSwitch* hw, RouteCounterID id, int modeId);
  virtual ~BcmRouteCounterBase() {}
  virtual BcmRouteCounterID getHwCounterID() const = 0;

 protected:
  BcmSwitch* hw_;
};

class BcmRouteCounter : public folly::MoveOnly, public BcmRouteCounterBase {
 public:
  BcmRouteCounter(BcmSwitch* hw, RouteCounterID id, int modeId);
  ~BcmRouteCounter() override;
  BcmRouteCounterID getHwCounterID() const override;

 private:
  BcmRouteCounterID hwCounterId_;
};

class BcmFlexCounterAction : public folly::MoveOnly {
 public:
  explicit BcmFlexCounterAction(BcmSwitch* hw) : hw_(hw) {}
  ~BcmFlexCounterAction();
  uint32_t getActionId() {
    return actionId_;
  }
  void setActionId(uint32_t id) {
    actionId_ = id;
  }
  void createFlexCounterAction();
  uint16_t allocateOffset();
  void clearOffset(uint16_t offset);
  void setOffset(uint16_t offset);

 private:
  uint32_t actionId_{0};
  static constexpr int kMaxFlexRouteCounters{1024};
  std::bitset<kMaxFlexRouteCounters> offsetMap_;
  BcmSwitch* hw_;
};

class BcmRouteFlexCounter : public folly::MoveOnly, public BcmRouteCounterBase {
 public:
  BcmRouteFlexCounter(
      BcmSwitch* hw,
      RouteCounterID id,
      std::shared_ptr<BcmFlexCounterAction> counterAction);
  ~BcmRouteFlexCounter() override;
  BcmRouteCounterID getHwCounterID() const override;

 private:
  BcmRouteCounterID hwCounterId_;
  std::shared_ptr<BcmFlexCounterAction> counterAction_;
};

class BcmRouteCounterTableBase {
 public:
  explicit BcmRouteCounterTableBase(BcmSwitch* hw) : hw_(hw) {}
  virtual ~BcmRouteCounterTableBase() {}
  void setMaxRouteCounterIDs(uint32_t count) {
    maxRouteCounterIDs_ = count;
  }
  virtual std::shared_ptr<BcmRouteCounterBase> referenceOrEmplaceCounterID(
      RouteCounterID id) = 0;
  // used for testing purpose
  virtual std::optional<BcmRouteCounterID> getHwCounterID(
      std::optional<RouteCounterID> counterID) const = 0;
  virtual folly::dynamic toFollyDynamic() const = 0;

  static constexpr folly::StringPiece kRouteCounters{"routeCounters"};
  static constexpr folly::StringPiece kRouteCounterIDs{"routeCounterIDs"};

 protected:
  BcmSwitch* hw_;
  uint32_t maxRouteCounterIDs_{0};
};

class BcmRouteFlexCounterTable : public folly::MoveOnly,
                                 public BcmRouteCounterTableBase {
 public:
  explicit BcmRouteFlexCounterTable(BcmSwitch* hw);
  ~BcmRouteFlexCounterTable() override {}
  std::optional<BcmRouteCounterID> getHwCounterID(
      std::optional<RouteCounterID> counterID) const override;
  std::shared_ptr<BcmRouteCounterBase> referenceOrEmplaceCounterID(
      RouteCounterID id) override;
  folly::dynamic toFollyDynamic() const override;
  std::shared_ptr<BcmFlexCounterAction> v6FlexCounterAction;
  static constexpr folly::StringPiece kFlexCounterActionV6{
      "flexCounterActionV6"};

 private:
  FlatRefMap<RouteCounterID, BcmRouteFlexCounter> counterIDs_;
};

class BcmRouteCounterTable : public folly::MoveOnly,
                             public BcmRouteCounterTableBase {
 public:
  explicit BcmRouteCounterTable(BcmSwitch* hw) : BcmRouteCounterTableBase(hw) {}
  std::optional<BcmRouteCounterID> getHwCounterID(
      std::optional<RouteCounterID> counterID) const override;
  ~BcmRouteCounterTable() override;
  std::shared_ptr<BcmRouteCounterBase> referenceOrEmplaceCounterID(
      RouteCounterID id) override;
  folly::dynamic toFollyDynamic() const override;
  static constexpr folly::StringPiece kGlobalModeId{"globalModeId"};

 private:
  FlatRefMap<RouteCounterID, BcmRouteCounter> counterIDs_;
  uint32_t createStatGroupModeId();
  int globalIngressModeId_{STAT_MODEID_INVALID};
};

} // namespace facebook::fboss
