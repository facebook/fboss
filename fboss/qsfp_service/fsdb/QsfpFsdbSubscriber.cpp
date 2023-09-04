// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/fsdb/QsfpFsdbSubscriber.h"
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook::fboss {

void QsfpFsdbSubscriber::subscribeToSwitchStatePortMap(
    TransceiverManager* /* tcvrManager */) {
  auto path = getSwitchStatePortMapPath();
  auto stateCb = [](fsdb::FsdbStreamClient::State /*old*/,
                    fsdb::FsdbStreamClient::State /*new*/) {};
  auto dataCb = [=](fsdb::OperState&& /* state */) {};
  pubSubMgr()->addStatePathSubscription(path, stateCb, dataCb);
  XLOG(INFO) << "Subscribed to switch state " << folly::join("/", path);
}

void QsfpFsdbSubscriber::removeSwitchStatePortMapSubscription() {
  auto path = getSwitchStatePortMapPath();
  pubSubMgr()->removeStatePathSubscription(path);
  XLOG(INFO) << "Unsubscribed from switch state " << folly::join("/", path);
}

// FSDB path:
//     FsdbOperStateRoot root()
//     .. AgentData agent()
//        .. switch_state.SwitchState() switchState()
//           .. portMaps.PortFields() portMaps()
std::vector<std::string> QsfpFsdbSubscriber::getSwitchStatePortMapPath() {
  thriftpath::RootThriftPath<fsdb::FsdbOperStateRoot> rootPath_;
  return rootPath_.agent().switchState().portMaps().tokens();
}

} // namespace facebook::fboss
