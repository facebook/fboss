// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include <folly/io/async/AsyncSocket.h>

#include <fboss/cli/fboss2/options/SSLPolicy.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/commands/show/hwagent/CmdShowHwAgentStatus.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

using namespace facebook::neteng::fboss::bgp::thrift;
extern std::vector<facebook::fboss::ArpEntryThrift> createArpEntries();

class MockAgentCounters : public AgentCountersIf {
 public:
  using FbSwHwAgentCounters = struct SwHwAgentCounters;
  using hostInfo = facebook::fboss::HostInfo;
  MOCK_METHOD(void, getAgentCounters, (hostInfo, int, FbSwHwAgentCounters&));
};

class MockFbossCtrlAgent : public FbossCtrlSvIf {
 public:
  MOCK_METHOD(void, reloadConfig, ());
  MOCK_METHOD(void, getAclTableGroup, (AclTableThrift&));
  MOCK_METHOD(void, getNdpTable, (std::vector<NdpEntryThrift>&));

  MOCK_METHOD(
      void,
      getArpTable,
      (std::vector<facebook::fboss::ArpEntryThrift>&));

  using PortInfoMap = std::map<int32_t, facebook::fboss::PortInfoThrift>&;
  using PortStatusMap = std::map<int32_t, facebook::fboss::PortStatus>&;
  using Out = std::string&;
  using Ports = std::unique_ptr<std::vector<int32_t>>;
  using HwObjects = std::unique_ptr<std::vector<HwObjectType>>;
  using HwAgentStatusMap =
      std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus>&;
  MOCK_METHOD(void, startPktCapture, (std::unique_ptr<CaptureInfo>));
  MOCK_METHOD(void, stopPktCapture, (std::unique_ptr<std::string>));
  MOCK_METHOD(void, getAllPortInfo, (PortInfoMap));
  MOCK_METHOD(void, getProductInfo, (ProductInfo&));
  MOCK_METHOD(
      void,
      getAllCpuPortStats,
      ((std::map<int32_t, facebook::fboss::CpuPortStats>&)));
  MOCK_METHOD(void, getPortStatus, (PortStatusMap, Ports));
  MOCK_METHOD(void, getHwAgentConnectionStatus, (HwAgentStatusMap));
  MOCK_METHOD(void, getMultiSwitchRunState, (MultiSwitchRunState&));
  MOCK_METHOD(void, listHwObjects, (Out, HwObjects, bool));
  MOCK_METHOD(SSLType, getSSLPolicy, ());
  MOCK_METHOD(void, setPortState, (int32_t, bool));
  MOCK_METHOD(
      void,
      getSwitchIdToSwitchInfo,
      ((std::map<int64_t, cfg::SwitchInfo>&)));
  MOCK_METHOD(
      void,
      getFabricConnectivity,
      ((std::map<std::string, FabricEndpoint>&)));
  MOCK_METHOD(
      void,
      getAggregatePortTable,
      (std::vector<facebook::fboss::AggregatePortThrift>&));
  MOCK_METHOD(
      void,
      getRouteTableDetails,
      (std::vector<facebook::fboss::RouteDetails>&));
  MOCK_METHOD(
      void,
      getRouteTable,
      (std::vector<facebook::fboss::UnicastRoute>&));
  MOCK_METHOD3(
      getIpRouteDetails,
      void(
          facebook::fboss::RouteDetails&,
          std::unique_ptr<facebook::network::thrift::Address>,
          int32_t));
  /* This unit test is a special case because the thrift spec for
  getRegexCounters uses "thread = eb".  This requires a pretty ugly mock
  definition and call to work */
  MOCK_METHOD2(
      async_eb_getRegexCounters,
      void(
          apache::thrift::HandlerCallbackPtr<
              std::unique_ptr<std::map<std::string, int64_t>>>,
          std::unique_ptr<std::string> regex));
  MOCK_METHOD(
      void,
      getTeFlowTableDetails,
      (std::vector<facebook::fboss::TeFlowDetails>&));
  MOCK_METHOD(
      void,
      addTeFlows,
      (std::unique_ptr<std::vector<FlowEntry>> teFlowEntries));
  MOCK_METHOD(
      void,
      deleteTeFlows,
      (std::unique_ptr<std::vector<TeFlow>> teFlows));
  MOCK_METHOD(void, getCurrentStateJSON, (Out, std::unique_ptr<std::string>));
  MOCK_METHOD(void, getRunningConfig, (std::string&));
  MOCK_METHOD(
      void,
      getAllEcmpDetails,
      (std::vector<facebook::fboss::EcmpDetails>&));
};

class MockFbossQsfpService : public QsfpServiceSvIf {
 public:
  using transceiverEntries =
      std::map<int32_t, facebook::fboss::TransceiverInfo>&;
  MOCK_METHOD2(
      getTransceiverInfo,
      void(transceiverEntries, std::unique_ptr<std::vector<int32_t>>));
  MOCK_METHOD3(
      getTransceiverConfigValidationInfo,
      void(
          std::map<int32_t, std::string>&,
          std::unique_ptr<std::vector<int32_t>>,
          bool));
};

class MockFbossBgpService : public TBgpServiceSvIf {
 public:
  MOCK_METHOD(void, getRunningConfig, (std::string&));
};
} // namespace facebook::fboss
