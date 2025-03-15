// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/fsdb/QsfpFsdbSubscriber.h"
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

namespace facebook::fboss {

void QsfpFsdbSubscriber::subscribeToSwitchStatePortMap(
    TransceiverManager* tcvrManager) {
  auto path = getSwitchStatePortMapPath();
  auto stateCb = [](fsdb::SubscriptionState /*old*/,
                    fsdb::SubscriptionState /*new*/,
                    std::optional<bool> /*initialSyncHasData*/) {};
  auto dataCb = [=](fsdb::OperState&& state) {
    if (auto contents = state.contents()) {
      using TC = apache::thrift::type_class::map<
          apache::thrift::type_class::string, // SwitchIdList
          apache::thrift::type_class::map<
              apache::thrift::type_class::integral,
              apache::thrift::type_class::structure>>;
      using T = std::map<
          fboss::state::SwitchIdList,
          std::map<int16_t, fboss::state::PortFields>>;
      auto swPortMaps = facebook::fboss::thrift_cow::deserialize<TC, T>(
          *state.protocol(), *state.contents());

      std::map<int, facebook::fboss::NpuPortStatus> newPortStatus;
      for (auto& [switchStr, oneSwPortMap] : swPortMaps) {
        for (auto& [onePortId, onePortInfo] : oneSwPortMap) {
          facebook::fboss::NpuPortStatus portStatus;
          portStatus.portId = onePortId;
          // TODO: We should not do the string comparison when portState is
          // changed to enum in switch_state
          portStatus.portEnabled =
              onePortInfo.portState().value() != "DISABLED";
          portStatus.operState = onePortInfo.portOperState().value();
          auto asicPrbs = onePortInfo.asicPrbs().value();
          portStatus.asicPrbsEnabled = asicPrbs.enabled().value();
          portStatus.profileID = onePortInfo.portProfileID().value();
          newPortStatus.emplace(onePortId, portStatus);
        }
      }
      tcvrManager->syncNpuPortStatusUpdate(newPortStatus);
    }
  };
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
