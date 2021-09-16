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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct BufferPoolCfgFields : public ThriftyFields {
  template <typename Fn>
  void forEachChild(Fn) {}

  state::BufferPoolFields toThrift() const;
  static BufferPoolCfgFields fromThrift(state::BufferPoolFields const&);

  std::string id;
  int sharedBytes;
  int headroomBytes;
};

/*
 * BufferPoolCfg stores the buffer pool related properites as need by {port, PG}
 */
class BufferPoolCfg : public ThriftyBaseT<
                          state::BufferPoolFields,
                          BufferPoolCfg,
                          BufferPoolCfgFields> {
 public:
  explicit BufferPoolCfg(const std::string& id) {
    writableFields()->id = id;
  }
  bool operator==(const BufferPoolCfg& cfg) const {
    return getFields()->sharedBytes == cfg.getSharedBytes() &&
        getFields()->headroomBytes == cfg.getHeadroomBytes();
  }

  bool operator!=(const BufferPoolCfg& cfg) const {
    return !(*this == cfg);
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

  void setHeadroomBytes(int headroomBytes) {
    writableFields()->headroomBytes = headroomBytes;
  }

  void setSharedBytes(int sharedBytes) {
    writableFields()->sharedBytes = sharedBytes;
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<
      state::BufferPoolFields,
      BufferPoolCfg,
      BufferPoolCfgFields>::ThriftyBaseT;
  friend class CloneAllocator;
};

using BufferPoolConfigs = std::vector<std::shared_ptr<BufferPoolCfg>>;
} // namespace facebook::fboss
