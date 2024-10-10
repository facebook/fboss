/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class PortApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    portApi = std::make_unique<PortApi>();
  }

  PortSaiId createPort(
      uint32_t speed,
      const std::vector<uint32_t>& lanes,
      bool adminState) const {
    SaiPortTraits::CreateAttributes a{
        lanes,        speed,        adminState,   std::nullopt,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
        std::nullopt, std::nullopt,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
        std::nullopt, // Port Fabric Isolate
#endif
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, // Ingress Mirror Session
        std::nullopt, // Egress Mirror Session
        std::nullopt, // Ingress Sample Packet
        std::nullopt, // Egress Sample Packet
        std::nullopt, // Ingress mirror sample session
        std::nullopt, // Egress mirror sample session
        std::nullopt, // PRBS Polynomial
        std::nullopt, // PRBS Config
        std::nullopt, // Ingress macsec acl
        std::nullopt, // Egress macsec acl
        std::nullopt, // System Port Id
        std::nullopt, // PTP Mode
        std::nullopt, // PFC Mode
        std::nullopt, // PFC Priorities
#if !defined(TAJO_SDK)
        std::nullopt, // PFC Rx Priorities
        std::nullopt, // PFC Tx Priorities
#endif
        std::nullopt, // TC to Priority Group map
        std::nullopt, // PFC Priority to Queue map
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
        std::nullopt, // Inter frame gap
#endif
        std::nullopt, // Link Training Enable
        std::nullopt, // FDR Enable
        std::nullopt, // Rx Lane Squelch Enable
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
        std::nullopt, // PFC Deadlock Detection Interval
        std::nullopt, // PFC Deadlock Recovery Interval
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
        std::nullopt, // ARS enable
        std::nullopt, // ARS scaling factor
        std::nullopt, // ARS port load past weight
        std::nullopt, // ARS port load future weight
#endif
        std::nullopt, // Reachability Group
    };
    return portApi->create<SaiPortTraits>(a, 0);
  }

  PortSerdesSaiId createPortSerdes(
      PortSaiId portSaiId,
      std::vector<sai_uint32_t> preemphasis,
      std::vector<sai_uint32_t> txPre1,
      std::vector<sai_uint32_t> txMain,
      std::vector<sai_uint32_t> txPost1,
      std::vector<sai_int32_t> rxCtlCode,
      std::vector<sai_int32_t> rxDspMode,
      std::vector<sai_int32_t> rxAfeTrim,
      std::vector<sai_int32_t> rxAcCouplingByPass,
      std::vector<sai_int32_t> rxAfeAdaptiveEnable,
      std::vector<sai_uint32_t> txPre3) const {
    SaiPortSerdesTraits::CreateAttributes a{
        portSaiId,
        preemphasis,
        std::nullopt, // IDriver
        txPre1,
        std::nullopt, // txPre2
        txPre3,
        txMain,
        txPost1,
        std::nullopt, // txPost2
        std::nullopt, // txPost3
        std::nullopt, // txLutMode
        rxCtlCode,
        rxDspMode,
        rxAfeTrim,
        rxAcCouplingByPass,
        rxAfeAdaptiveEnable,
        std::nullopt, // txDiffEncoderEn
        std::nullopt, // txDigGain
        std::nullopt, // txFfeCoeff0
        std::nullopt, // txFfeCoeff1
        std::nullopt, // txFfeCoeff2
        std::nullopt, // txFfeCoeff3
        std::nullopt, // txFfeCoeff4
        std::nullopt, // txDriverSwing
        std::nullopt, // rxInstgBoost1Start
        std::nullopt, // rxInstgBoost1Step
        std::nullopt, // rxInstgBoost1Stop
        std::nullopt, // rxInstgBoost2OrHrStart
        std::nullopt, // rxInstgBoost2OrHrStep
        std::nullopt, // rxInstgBoost2OrHrStop
        std::nullopt, // rxInstgC1Start1p7
        std::nullopt, // rxInstgC1Step1p7
        std::nullopt, // rxInstgC1Stop1p7
        std::nullopt, // rxInstgDfeStart1p7
        std::nullopt, // rxInstgDfeStep1p7
        std::nullopt, // rxInstgDfeStop1p7
        std::nullopt, // rxEnableScanSelection
        std::nullopt, // rxInstgScanUseSrSettings
        std::nullopt, // rxCdrCfgOvEn
        std::nullopt, // rxCdrTdet1stOrdStepOvVal
        std::nullopt, // rxCdrTdet2ndOrdStepOvVal
        std::nullopt, // rxCdrTdetFineStepOvVal
    };
    return portApi->create<SaiPortSerdesTraits>(a, 0 /*switch id*/);
  }

  std::vector<PortSaiId> createFivePorts() const {
    std::vector<PortSaiId> portIds;
    portIds.push_back(createPort(100000, {0, 1, 2, 3}, true));
    for (uint32_t i = 4; i < 8; ++i) {
      portIds.push_back(createPort(25000, {i}, false));
    }
    for (const auto& portId : portIds) {
      checkPort(portId);
    }
    return portIds;
  }

  void checkPort(PortSaiId portId) const {
    SaiPortTraits::Attributes::AdminState adminStateAttribute;
    std::vector<uint32_t> ls;
    ls.resize(4);
    SaiPortTraits::Attributes::HwLaneList hwLaneListAttribute(ls);
    SaiPortTraits::Attributes::Speed speedAttribute;
    auto gotAdminState = portApi->getAttribute(portId, adminStateAttribute);
    auto gotSpeed = portApi->getAttribute(portId, speedAttribute);
    auto lanes = portApi->getAttribute(portId, hwLaneListAttribute);
    EXPECT_EQ(fs->portManager.get(portId).adminState, gotAdminState);
    EXPECT_EQ(fs->portManager.get(portId).speed, gotSpeed);
    EXPECT_EQ(fs->portManager.get(portId).id, portId);
    EXPECT_EQ(fs->portManager.get(portId).lanes, lanes);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<PortApi> portApi;
};

TEST_F(PortApiTest, onePort) {
  auto id = createPort(100000, {42}, true);
  SaiPortTraits::Attributes::AdminState as_blank;
  std::vector<uint32_t> ls;
  ls.resize(1);
  SaiPortTraits::Attributes::HwLaneList l_blank(ls);
  SaiPortTraits::Attributes::Speed s_blank;
  EXPECT_EQ(portApi->getAttribute(id, as_blank), true);
  EXPECT_EQ(portApi->getAttribute(id, s_blank), 100000);
  auto lanes = portApi->getAttribute(id, l_blank);
  EXPECT_EQ(lanes.size(), 1);
  EXPECT_EQ(lanes[0], 42);
}

TEST_F(PortApiTest, fourPorts) {
  auto portIds = createFivePorts();
}

TEST_F(PortApiTest, setPortAttributes) {
  auto portIds = createFivePorts();

  SaiPortTraits::Attributes::AdminState as_attr(true);
  SaiPortTraits::Attributes::Speed speed_attr(50000);
  // set speeds
  portApi->setAttribute(portIds[0], speed_attr);
  portApi->setAttribute(portIds[2], speed_attr);
  // set admin state
  portApi->setAttribute(portIds[2], as_attr);
  // confirm admin states
  EXPECT_EQ(portApi->getAttribute(portIds[0], as_attr), true);
  EXPECT_EQ(portApi->getAttribute(portIds[1], as_attr), false);
  EXPECT_EQ(portApi->getAttribute(portIds[2], as_attr), true);
  EXPECT_EQ(portApi->getAttribute(portIds[3], as_attr), false);
  // confirm speeds
  EXPECT_EQ(portApi->getAttribute(portIds[0], speed_attr), 50000);
  EXPECT_EQ(portApi->getAttribute(portIds[1], speed_attr), 25000);
  EXPECT_EQ(portApi->getAttribute(portIds[2], speed_attr), 50000);
  EXPECT_EQ(portApi->getAttribute(portIds[3], speed_attr), 25000);
  // confirm consistency internally, too
  for (const auto& portId : portIds) {
    checkPort(portId);
  }
}

TEST_F(PortApiTest, removePort) {
  {
    // basic remove
    auto portId = createPort(25000, {42}, true);
    portApi->remove(portId);
  }
  {
    // remove a const portId
    const auto portId = createPort(25000, {42}, true);
    portApi->remove((portId));
  }
  {
    // remove rvalue id
    portApi->remove(createPort(25000, {42}, true));
  }
  {
    // remove in "canonical" for-loop
    auto portIds = createFivePorts();
    for (const auto& portId : portIds) {
      portApi->remove(portId);
    }
  }
}

TEST_F(PortApiTest, getLaneListPresized) {
  std::vector<uint32_t> inLanes{0, 1, 2, 3};
  auto portId = createPort(100000, inLanes, true);
  std::vector<uint32_t> tempLanes;
  tempLanes.resize(4);
  SaiPortTraits::Attributes::HwLaneList hwLaneListAttr{tempLanes};
  auto gotLanes = portApi->getAttribute(portId, hwLaneListAttr);
  EXPECT_EQ(gotLanes, inLanes);
}

TEST_F(PortApiTest, getLaneListUnsized) {
  std::vector<uint32_t> inLanes{0, 1, 2, 3};
  auto portId = createPort(100000, inLanes, true);
  SaiPortTraits::Attributes::HwLaneList hwLaneListAttr;
  auto gotLanes = portApi->getAttribute(portId, hwLaneListAttr);
  EXPECT_EQ(gotLanes, inLanes);
}

TEST_F(PortApiTest, setGetOptionalAttributes) {
  auto portId = createPort(25000, {42}, true);

  // Fec Mode get/set
  int32_t saiFecMode = SAI_PORT_FEC_MODE_RS;
  SaiPortTraits::Attributes::FecMode fecMode{saiFecMode};
  portApi->setAttribute(portId, fecMode);
  auto gotFecMode = portApi->getAttribute(portId, fecMode);
  EXPECT_EQ(gotFecMode, saiFecMode);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  // Internal Loopback Mode get/set
  int32_t saiLoopbackMode = SAI_PORT_LOOPBACK_MODE_MAC;
  SaiPortTraits::Attributes::PortLoopbackMode loopbackMode{saiLoopbackMode};
  portApi->setAttribute(portId, loopbackMode);
  auto gotLoopbackMode = portApi->getAttribute(portId, loopbackMode);
  EXPECT_EQ(gotLoopbackMode, saiLoopbackMode);
#endif

  // Internal Loopback Mode get/set
  int32_t saiInternalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC;
  SaiPortTraits::Attributes::InternalLoopbackMode internalLoopbackMode{
      saiInternalLoopbackMode};
  portApi->setAttribute(portId, internalLoopbackMode);
  auto gotInternalLoopbackMode =
      portApi->getAttribute(portId, internalLoopbackMode);
  EXPECT_EQ(gotInternalLoopbackMode, saiInternalLoopbackMode);

  // Media type get/set
  int32_t saiMediaType = SAI_PORT_MEDIA_TYPE_COPPER;
  SaiPortTraits::Attributes::MediaType mediaType{saiMediaType};
  portApi->setAttribute(portId, mediaType);
  auto gotMediaType = portApi->getAttribute(portId, mediaType);
  EXPECT_EQ(gotMediaType, saiMediaType);

  // Global Flow Control get/set
  int32_t saiFlowControl = SAI_PORT_FLOW_CONTROL_MODE_RX_ONLY;
  SaiPortTraits::Attributes::GlobalFlowControlMode flowControl{saiFlowControl};
  portApi->setAttribute(portId, flowControl);
  auto gotFlowControl = portApi->getAttribute(portId, flowControl);
  EXPECT_EQ(gotFlowControl, saiFlowControl);

  // Ingress port vlan get/set
  sai_vlan_id_t saiPortVlanId = 2000;
  SaiPortTraits::Attributes::PortVlanId portVlanId{saiPortVlanId};
  portApi->setAttribute(portId, portVlanId);
  auto gotPortVlanId = portApi->getAttribute(portId, portVlanId);
  EXPECT_EQ(gotPortVlanId, saiPortVlanId);

  // Port MTU
  sai_uint32_t mtu{9000};
  SaiPortTraits::Attributes::Mtu portMtu{mtu};
  portApi->setAttribute(portId, portMtu);
  auto gotPortMtu = portApi->getAttribute(portId, portMtu);
  EXPECT_EQ(gotPortMtu, mtu);

  // Port DSCP to TC
  sai_object_id_t qosMapDscpToTc{42};
  SaiPortTraits::Attributes::QosDscpToTcMap portDscpToTc{qosMapDscpToTc};
  portApi->setAttribute(portId, portDscpToTc);
  auto gotPortDscpToTc = portApi->getAttribute(portId, portDscpToTc);
  EXPECT_EQ(gotPortDscpToTc, qosMapDscpToTc);

  // Port TC to queue
  sai_object_id_t qosMapTcToQueue{43};
  SaiPortTraits::Attributes::QosTcToQueueMap portTcToQueue{qosMapTcToQueue};
  portApi->setAttribute(portId, portTcToQueue);
  auto gotPortTcToQueue = portApi->getAttribute(portId, portTcToQueue);
  EXPECT_EQ(gotPortTcToQueue, qosMapTcToQueue);

  // Port TTL decrement
  SaiPortTraits::Attributes::DisableTtlDecrement disableTtlDec{true};
  portApi->setAttribute(portId, disableTtlDec);
  EXPECT_TRUE(portApi->getAttribute(portId, disableTtlDec));

  // Pkt TX
  SaiPortTraits::Attributes::PktTxEnable txEnable{false};
  portApi->setAttribute(portId, txEnable);
  EXPECT_FALSE(portApi->getAttribute(portId, txEnable));

  // System port ID
  uint16_t systemPortId{1001};
  portApi->setAttribute(
      portId, SaiPortTraits::Attributes::SystemPortId{systemPortId});
  auto gotSystemPortId =
      portApi->getAttribute(portId, SaiPortTraits::Attributes::SystemPortId{});
  EXPECT_EQ(gotSystemPortId, systemPortId);

  // Prbs Polynomial
  uint32_t prbsPolynomial{42};
  portApi->setAttribute(
      portId, SaiPortTraits::Attributes::PrbsPolynomial{prbsPolynomial});
  auto gotPrbsPolynomial = portApi->getAttribute(
      portId, SaiPortTraits::Attributes::PrbsPolynomial{});
  EXPECT_EQ(gotPrbsPolynomial, prbsPolynomial);

  // Prbs Config
  int32_t prbsConfig = SAI_PORT_PRBS_CONFIG_ENABLE_TX_RX;
  portApi->setAttribute(
      portId, SaiPortTraits::Attributes::PrbsConfig{prbsConfig});
  auto gotPrbsConfig =
      portApi->getAttribute(portId, SaiPortTraits::Attributes::PrbsConfig{});
  EXPECT_EQ(gotPrbsConfig, prbsConfig);

  // PTP Mode get/set
  int32_t saiPtpMode = SAI_PORT_PTP_MODE_NONE;
  SaiPortTraits::Attributes::PtpMode ptpMode{saiPtpMode};
  portApi->setAttribute(portId, ptpMode);
  auto gotPtpMode = portApi->getAttribute(portId, ptpMode);
  EXPECT_EQ(gotPtpMode, saiPtpMode);

#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  // Inter frame gap
  uint32_t interFrameGap = 352;
  portApi->setAttribute(
      portId, SaiPortTraits::Attributes::InterFrameGap{interFrameGap});
  auto gotInterFrameGap =
      portApi->getAttribute(portId, SaiPortTraits::Attributes::InterFrameGap{});
  EXPECT_EQ(interFrameGap, gotInterFrameGap);
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
  // Port Fabric Isolate
  SaiPortTraits::Attributes::FabricIsolate fabricIsolate_attr(true);
  portApi->setAttribute(portId, fabricIsolate_attr);
  EXPECT_EQ(portApi->getAttribute(portId, fabricIsolate_attr), true);
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  // ARS related attributes
  // Ars enable
  SaiPortTraits::Attributes::ArsEnable arsEnable_attr(true);
  portApi->setAttribute(portId, arsEnable_attr);
  EXPECT_EQ(portApi->getAttribute(portId, arsEnable_attr), true);
  // Ars scaling factor
  SaiPortTraits::Attributes::ArsPortLoadScalingFactor
      arsPortLoadScalingFactor_attr(16);
  portApi->setAttribute(portId, arsPortLoadScalingFactor_attr);
  EXPECT_EQ(portApi->getAttribute(portId, arsPortLoadScalingFactor_attr), 16);
  // Ars port load past weight
  SaiPortTraits::Attributes::ArsPortLoadPastWeight arsPortLoadPastWeight_attr(
      60);
  portApi->setAttribute(portId, arsPortLoadPastWeight_attr);
  EXPECT_EQ(portApi->getAttribute(portId, arsPortLoadPastWeight_attr), 60);
  // Ars port load future weight
  SaiPortTraits::Attributes::ArsPortLoadFutureWeight
      arsPortLoadFutureWeight_attr(20);
  portApi->setAttribute(portId, arsPortLoadFutureWeight_attr);
  EXPECT_EQ(portApi->getAttribute(portId, arsPortLoadFutureWeight_attr), 20);
#endif
}

// ObjectApi tests
TEST_F(PortApiTest, portCount) {
  createFivePorts();
  auto count = getObjectCount<SaiPortTraits>(0);
  EXPECT_EQ(count, 5);
}

TEST_F(PortApiTest, portKeys) {
  auto portIds = createFivePorts();
  auto keys = getObjectKeys<SaiPortTraits>(0);
  EXPECT_EQ(keys.size(), 5);
  std::sort(portIds.begin(), portIds.end());
  std::sort(keys.begin(), keys.end());
  EXPECT_EQ(keys, portIds);
}

TEST_F(PortApiTest, getAllStats) {
  auto id = createPort(100000, {42}, true);
  auto stats = portApi->getStats<SaiPortTraits>(id, SAI_STATS_MODE_READ);
  EXPECT_EQ(stats.size(), SaiPortTraits::CounterIdsToRead.size());
}

TEST_F(PortApiTest, getSome) {
  auto id = createPort(100000, {42}, true);
  auto stats = portApi->getStats<SaiPortTraits>(
      id,
      {SAI_PORT_STAT_IF_IN_OCTETS, SAI_PORT_STAT_IF_IN_UCAST_PKTS},
      SAI_STATS_MODE_READ);
  EXPECT_EQ(stats.size(), 2);
}

TEST_F(PortApiTest, serdesApi) {
  auto id = createPort(100000, {42}, true);
  auto serdesId =
      createPortSerdes(id, {0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9});
  auto preemphasis = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::Preemphasis{});
  auto txFirPre1 = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::TxFirPre1{});
  auto txFirPre3 = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::TxFirPre3{});
  auto txFirMain = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::TxFirMain{});
  auto txFirPost1 = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::TxFirPost1{});
  auto rxCtleCode = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::RxCtleCode{});
  auto rxDspMode = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::RxDspMode{});
  auto rxAfeTrim = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::RxAfeTrim{});
  auto rxAcCouplingByPass = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{});
  auto rxAfeAdaptiveEnable = portApi->getAttribute(
      serdesId, SaiPortSerdesTraits::Attributes::RxAfeAdaptiveEnable{});
  EXPECT_EQ(preemphasis, std::vector<sai_uint32_t>{0});
  EXPECT_EQ(txFirPre1, std::vector<sai_uint32_t>{1});
  EXPECT_EQ(txFirMain, std::vector<sai_uint32_t>{2});
  EXPECT_EQ(txFirPost1, std::vector<sai_uint32_t>{3});
  EXPECT_EQ(rxCtleCode, std::vector<sai_int32_t>{4});
  EXPECT_EQ(rxDspMode, std::vector<sai_int32_t>{5});
  EXPECT_EQ(rxAfeTrim, std::vector<sai_int32_t>{6});
  EXPECT_EQ(rxAcCouplingByPass, std::vector<sai_int32_t>{7});
  EXPECT_EQ(rxAfeAdaptiveEnable, std::vector<sai_int32_t>{8});
  EXPECT_EQ(txFirPre3, std::vector<sai_uint32_t>{9});
}

#if !defined(IS_OSS)
// interface modes used by fb, not available in OSS yet
TEST_F(PortApiTest, setInterfaceType) {
  auto id = createPort(100000, {42}, true);
  SaiPortTraits::Attributes::InterfaceType interface_type{
      SAI_PORT_INTERFACE_TYPE_CAUI};
  portApi->setAttribute(id, interface_type);
  EXPECT_EQ(
      portApi->getAttribute(id, SaiPortTraits::Attributes::InterfaceType{}),
      SAI_PORT_INTERFACE_TYPE_CAUI);
}
#endif

TEST_F(PortApiTest, getFabricAttachedSwitchId) {
  auto id = createPort(100000, {42}, true);
  auto swId = portApi->getAttribute(
      id, SaiPortTraits::Attributes::FabricAttachedSwitchId{});
  EXPECT_EQ(swId, 0);
}

TEST_F(PortApiTest, getFabricAttached) {
  auto id = createPort(100000, {42}, true);
  EXPECT_FALSE(
      portApi->getAttribute(id, SaiPortTraits::Attributes::FabricAttached{}));
}

TEST_F(PortApiTest, getFabricAttachedPortIndex) {
  auto id = createPort(100000, {42}, true);
  EXPECT_FALSE(portApi->getAttribute(
      id, SaiPortTraits::Attributes::FabricAttachedPortIndex{}));
}

TEST_F(PortApiTest, getFabricAttachedSwitchType) {
  auto id = createPort(100000, {42}, true);
  auto swType = portApi->getAttribute(
      id, SaiPortTraits::Attributes::FabricAttachedSwitchType{});
  EXPECT_EQ(swType, SAI_SWITCH_TYPE_VOQ);
}

TEST_F(PortApiTest, getFabricReachability) {
  const auto switchId = 3;
  auto id = createPort(100000, {42}, true);
  sai_fabric_port_reachability_t reachability;
  reachability.switch_id = switchId;
  auto reachabilityGot = portApi->getAttribute(
      id, SaiPortTraits::Attributes::FabricReachability{reachability});
  EXPECT_EQ(reachabilityGot.switch_id, switchId);
  EXPECT_TRUE(reachabilityGot.reachable);
}
