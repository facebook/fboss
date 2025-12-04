// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/fsdb/QsfpFsdbSubscriber.h"
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

DEFINE_bool(
    enable_fsdb_patch_subscriber,
    false,
    "Enable FSDB subscriptions using patches, default false");

namespace {
// FSDB path:
//     FsdbOperStateRoot root()
//     .. AgentData agent()
//        .. switch_state.SwitchState() switchState()
//           .. portMaps.PortFields() portMaps()
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    fsdbStateRootPath;
const auto switchStatePortMapPath =
    fsdbStateRootPath.agent().switchState().portMaps();
} // namespace

namespace facebook::fboss {

QsfpFsdbSubscriber::QsfpFsdbSubscriber()
    : fsdbPubSubMgr_{FLAGS_enable_fsdb_patch_subscriber ? nullptr : std::make_unique<fsdb::FsdbPubSubManager>("qsfp_service")},
      fsdbSubMgr_{
          FLAGS_enable_fsdb_patch_subscriber
              ? std::make_unique<fsdb::FsdbCowStateSubManager>(
                    fsdb::SubscriptionOptions("qsfp_service"))
              : nullptr} {}

void QsfpFsdbSubscriber::subscribeToSwitchStatePortMap(
    TransceiverManager* tcvrManager,
    PortManager* portManager) {
  const auto& path = switchStatePortMapPath;
  using T = std::map<
      fboss::state::SwitchIdList,
      std::map<int16_t, fboss::state::PortFields>>;
  auto processData = [tcvrManager, portManager](const T& swPortMaps) {
    std::map<int, facebook::fboss::NpuPortStatus> newPortStatus;
    for (auto& [switchStr, oneSwPortMap] : swPortMaps) {
      for (auto& [onePortId, onePortInfo] : oneSwPortMap) {
        facebook::fboss::NpuPortStatus portStatus;
        portStatus.portId = onePortId;
        // TODO: We should not do the string comparison when portState is
        // changed to enum in switch_state
        portStatus.portEnabled = onePortInfo.portState().value() != "DISABLED";
        portStatus.operState = onePortInfo.portOperState().value();
        auto asicPrbs = onePortInfo.asicPrbs().value();
        portStatus.asicPrbsEnabled = asicPrbs.enabled().value();
        portStatus.profileID = onePortInfo.portProfileID().value();
        newPortStatus.emplace(onePortId, portStatus);
      }
    }
    if (FLAGS_port_manager_mode) {
      portManager->syncNpuPortStatusUpdate(newPortStatus);
    } else {
      tcvrManager->syncNpuPortStatusUpdate(newPortStatus);
    }
  };

  if (fsdbPubSubMgr_) {
    auto stateCb = [](fsdb::SubscriptionState /*old*/,
                      fsdb::SubscriptionState /*new*/,
                      std::optional<bool> /*initialSyncHasData*/) {};
    auto dataCb = [processData =
                       std::move(processData)](fsdb::OperState&& state) {
      if (auto contents = state.contents()) {
        using TC = apache::thrift::type_class::map<
            apache::thrift::type_class::string, // SwitchIdList
            apache::thrift::type_class::map<
                apache::thrift::type_class::integral,
                apache::thrift::type_class::structure>>;
        auto swPortMaps = facebook::fboss::thrift_cow::deserialize<TC, T>(
            *state.protocol(), *state.contents());
        processData(swPortMaps);
      }
    };
    fsdbPubSubMgr_->addStatePathSubscription(path.tokens(), stateCb, dataCb);
  } else {
    CHECK(fsdbSubMgr_);
    fsdbSubMgr_->addPath(path);
    fsdbSubMgr_->subscribe([processData = std::move(processData)](auto update) {
      auto swPortMaps =
          *update.data->toThrift().agent()->switchState()->portMaps();
      XLOG(DBG5)
          << "QsfpFsdbSubscriber: patch: swPortMaps: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 swPortMaps);
      processData(swPortMaps);
    });
  }

  XLOG(INFO) << "QsfpFsdbSubscriber: subscribed to switch state "
             << folly::join("/", path.tokens())
             << " using patch=" << FLAGS_enable_fsdb_patch_subscriber;
}

void QsfpFsdbSubscriber::stop() {
  XLOG(INFO) << "Stopping QsfpFsdbSubscriber";
  if (fsdbPubSubMgr_) {
    fsdbPubSubMgr_->removeStatePathSubscription(
        switchStatePortMapPath.tokens());
  }
  if (fsdbSubMgr_) {
    fsdbSubMgr_->stop();
  }
  XLOG(INFO) << "QsfpFsdbSubscriber stopped";
}

} // namespace facebook::fboss
