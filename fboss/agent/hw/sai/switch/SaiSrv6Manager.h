// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#endif

#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
using SaiSrv6SidList = SaiObject<SaiSrv6SidListTraits>;
#endif

struct SaiSrv6SidListHandle {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidList> sidList;
#endif
};

class SaiSrv6Manager {
 public:
  SaiSrv6Manager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidListHandle> addOrReuseSrv6SidList(
      const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey,
      const SaiSrv6SidListTraits::CreateAttributes& createAttributes);

  const SaiSrv6SidListHandle* getSrv6SidListHandle(
      const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey) const;
#endif

 private:
  SaiStore* saiStore_;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  UnorderedRefMap<SaiSrv6SidListTraits::AdapterHostKey, SaiSrv6SidListHandle>
      srv6SidLists_;
#endif
};

} // namespace facebook::fboss
