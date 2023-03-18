// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include <fboss/cli/fboss2/options/SSLPolicy.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace ::testing;
namespace facebook::fboss {

extern std::vector<facebook::fboss::ArpEntryThrift> createArpEntries();

class MockFbossCtrlAgent : public FbossCtrlSvIf {
 public:
  MOCK_METHOD(void, getAclTable, (std::vector<AclEntryThrift>&));
  MOCK_METHOD(void, getL2Table, (std::vector<L2EntryThrift>&));
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
  MOCK_METHOD(void, startPktCapture, (std::unique_ptr<CaptureInfo>));
  MOCK_METHOD(void, stopPktCapture, (std::unique_ptr<std::string>));
  MOCK_METHOD(void, getAllPortInfo, (PortInfoMap));
  MOCK_METHOD(void, getPortStatus, (PortStatusMap, Ports));
  MOCK_METHOD(void, listHwObjects, (Out, HwObjects, bool));
  MOCK_METHOD(SSLType, getSSLPolicy, ());
  MOCK_METHOD(void, setPortState, (int32_t, bool));
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
  /* This unit test is a special case because the thrift spec for
  getRegexCounters uses "thread = eb".  This requires a pretty ugly mock
  definition and call to work */
  MOCK_METHOD2(
      async_eb_getRegexCounters,
      void(
          std::unique_ptr<apache::thrift::HandlerCallback<
              std::unique_ptr<std::map<std::string, int64_t>>>>,
          std::unique_ptr<std::string> regex));
  MOCK_METHOD(
      void,
      getTeFlowTableDetails,
      (std::vector<facebook::fboss::TeFlowDetails>&));
  MOCK_METHOD(void, getCurrentStateJSON, (Out, std::unique_ptr<std::string>));
};

class MockFbossQsfpService : public QsfpServiceSvIf {
 public:
  using transceiverEntries =
      std::map<int32_t, facebook::fboss::TransceiverInfo>&;
  MOCK_METHOD2(
      getTransceiverInfo,
      void(transceiverEntries, std::unique_ptr<std::vector<int32_t>>));
};
} // namespace facebook::fboss
