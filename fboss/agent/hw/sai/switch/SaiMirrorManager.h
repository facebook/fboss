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

#include "fboss/agent/hw/sai/api/MirrorApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiMirror2Port = SaiObject<SaiLocalMirrorTraits>;
using SaiMirror2GreTunnel = SaiObject<SaiEnhancedRemoteMirrorTraits>;

struct SaiMirrorHandle {
  using SaiMirror = std::variant<
      std::shared_ptr<SaiMirror2Port>,
      std::shared_ptr<SaiMirror2GreTunnel>>;
  SaiMirror mirror;
};

class SaiMirrorManager {
 public:
  SaiMirrorManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  void addMirror(const std::shared_ptr<Mirror>& mirror);
  void removeMirror(const std::shared_ptr<Mirror>& mirror);
  void changeMirror(
      const std::shared_ptr<Mirror>& oldMirror,
      const std::shared_ptr<Mirror>& newMirror);
  const SaiMirrorHandle* FOLLY_NULLABLE
  getMirrorHandle(const std::string& mirrorId) const;
  SaiMirrorHandle* FOLLY_NULLABLE getMirrorHandle(const std::string& mirrorId);

 private:
  SaiMirrorHandle* FOLLY_NULLABLE
  getMirrorHandleImpl(const std::string& mirrorId) const;
  SaiManagerTable* managerTable_;
  folly::F14FastMap<std::string, std::unique_ptr<SaiMirrorHandle>>
      mirrorHandles_;
  SaiMirrorHandle::SaiMirror addMirrorSpan(PortSaiId monitorPort);
  SaiMirrorHandle::SaiMirror addMirrorErSpan(
      const std::shared_ptr<Mirror>& mirror,
      PortSaiId monitorPort);
};

} // namespace facebook::fboss
