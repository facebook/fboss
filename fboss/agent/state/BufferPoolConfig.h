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

struct BufferPoolCfgFields {
  BufferPoolCfgFields(
      const std::string& id,
      const int sharedBytes,
      const int headroomBytes)
      : id(id), sharedBytes(sharedBytes), headroomBytes(headroomBytes) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static BufferPoolCfgFields fromFollyDynamic(const folly::dynamic& json);

  const std::string id;
  int sharedBytes;
  int headroomBytes;
};

/*
 * BufferPoolCfg stores the buffer pool related properites as need by {port, PG}
 */
class BufferPoolCfg : public NodeBaseT<BufferPoolCfg, BufferPoolCfgFields> {
 public:
  BufferPoolCfg(
      const std::string& id,
      const int sharedBytes,
      const int headroomBytes);

  static std::shared_ptr<BufferPoolCfg> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = BufferPoolCfgFields::fromFollyDynamic(json);
    return std::make_shared<BufferPoolCfg>(fields);
  }

  static std::shared_ptr<BufferPoolCfg> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const BufferPoolCfg& cfg) const {
    return getFields()->sharedBytes == cfg.getSharedBytes() &&
        getFields()->headroomBytes == cfg.getHeadroomBytes();
  }

  bool operator!=(const BufferPoolCfg& cfg) const {
    return !operator==(cfg);
  }

  int getSharedBytes() const {
    return getFields()->sharedBytes;
  }

  int getHeadroomBytes() const {
    return getFields()->headroomBytes;
  }

  const std::string& getID() const {
    return getFields()->id;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
