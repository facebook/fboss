// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchInfoTable.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

template <typename SwitchTypeT>
class SwitchIdScopeResolverTest : public ::testing::Test {
 public:
  static auto constexpr switchType = SwitchTypeT::switchType;
  void SetUp() override {
    auto config = testConfigA(SwitchTypeT::switchType);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }

 protected:
  HwSwitchMatcher l3SwitchMatcher() const {
    auto l3SwitchType = switchInfo().l3SwitchType();
    auto l3SwitchIds = switchInfo().getSwitchIdsOfType(l3SwitchType);
    return HwSwitchMatcher(l3SwitchIds);
  }
  HwSwitchMatcher voqSwitchMatcher() const {
    return HwSwitchMatcher(
        switchInfo().getSwitchIdsOfType(cfg::SwitchType::VOQ));
  }
  HwSwitchMatcher allSwitchMatcher() const {
    return HwSwitchMatcher(switchInfo().getSwitchIDs());
  }
  const SwitchIdScopeResolver& scopeResolver() const {
    return *sw_->getScopeResolver();
  }
  template <typename... Args>
  void expectThrow(Args&&... args) {
    EXPECT_THROW(
        scopeResolver().scope(std::forward<Args>(args)...), FbossError);
  }
  template <typename... Args>
  void expectAll(Args&&... args) {
    EXPECT_EQ(
        scopeResolver().scope(std::forward<Args>(args)...), allSwitchMatcher());
  }
  template <typename... Args>
  void expectVoq(Args&&... args) {
    EXPECT_EQ(
        scopeResolver().scope(std::forward<Args>(args)...), voqSwitchMatcher());
  }
  template <typename... Args>
  void expectL3(Args&&... args) {
    EXPECT_EQ(
        scopeResolver().scope(std::forward<Args>(args)...), l3SwitchMatcher());
  }
  template <typename... Args>
  void expectSwitchId(Args&&... args) {
    auto switchIds = switchInfo().getSwitchIDs();
    ASSERT_EQ(switchIds.size(), 1);
    EXPECT_EQ(
        scopeResolver().scope(std::forward<Args>(args)...),
        HwSwitchMatcher(switchIds));
  }

  bool isVoq() const {
    return switchType == cfg::SwitchType::VOQ;
  }
  bool isFabric() const {
    return switchType == cfg::SwitchType::FABRIC;
  }
  bool isNpu() const {
    return switchType == cfg::SwitchType::NPU;
  }

  const SwitchInfoTable& switchInfo() const {
    return sw_->getSwitchInfoTable();
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TYPED_TEST_SUITE(SwitchIdScopeResolverTest, SwitchTypes);

TYPED_TEST(SwitchIdScopeResolverTest, mirrorScope) {
  if (this->isFabric()) {
    this->expectThrow(cfg::Mirror{});
    this->expectThrow(std::shared_ptr<Mirror>{});
  } else {
    this->expectL3(cfg::Mirror{});
    this->expectL3(std::shared_ptr<Mirror>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, dsfNodeScope) {
  this->expectAll(cfg::DsfNode{});
  this->expectAll(std::shared_ptr<DsfNode>());
}

TYPED_TEST(SwitchIdScopeResolverTest, portScope) {
  this->expectSwitchId(PortID(6));
}

TYPED_TEST(SwitchIdScopeResolverTest, portsScope) {
  this->expectSwitchId(std::vector<PortID>({PortID(6), PortID(8)}));
}

TYPED_TEST(SwitchIdScopeResolverTest, portObjScope) {
  state::PortFields portFields;
  portFields.portId() = PortID(1);
  portFields.portName() = "port1";
  auto port = std::make_shared<Port>(std::move(portFields));
  this->expectSwitchId(port);
}

TYPED_TEST(SwitchIdScopeResolverTest, portcfgScope) {
  cfg::Port port;
  port.logicalID() = 1;
  this->expectSwitchId(port);
}

TYPED_TEST(SwitchIdScopeResolverTest, aggPortScope) {
  cfg::AggregatePort aggPort;
  aggPort.name() = "agg";
  cfg::AggregatePortMember member;
  member.memberPortID() = 6;
  aggPort.memberPorts()->push_back(member);
  if (this->isFabric()) {
    this->expectThrow(aggPort);
  } else {
    this->expectL3(aggPort);
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, sysPortScope) {
  if (this->isVoq()) {
    this->expectVoq(SystemPortID(101));
    this->expectVoq(SystemPortID(101));
    this->expectVoq(std::make_shared<SystemPort>(SystemPortID(101)));
    this->expectVoq(SystemPortID(1001));
    this->expectVoq(std::make_shared<SystemPort>(SystemPortID(1001)));
  } else {
    this->expectThrow(SystemPortID(101));
    this->expectThrow(SystemPortID(101));
    this->expectThrow(std::make_shared<SystemPort>(SystemPortID(101)));
    this->expectThrow(SystemPortID(1001));
    this->expectThrow(std::make_shared<SystemPort>(SystemPortID(1001)));
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, vlanScope) {
  auto vlan1 = std::make_shared<Vlan>(VlanID(1), std::string("Vlan1"));
  vlan1->setPorts({{0, true}});
  this->expectSwitchId(vlan1);
  auto vlan2 = std::make_shared<Vlan>(VlanID(2), std::string("Vlan2"));
  this->expectAll(vlan2);
}

TYPED_TEST(SwitchIdScopeResolverTest, interfaceScope) {
  if (this->isFabric()) {
    return;
  } else {
    auto state = this->sw_->getState();
    auto allIntfs = state->getInterfaces();
    auto intf = allIntfs->getAllNodes()->cbegin()->second;
    this->expectL3(intf, state);
    this->expectL3(intf, this->sw_->getConfig());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, fibContainerScope) {
  auto fibContainer = std::make_shared<ForwardingInformationBaseContainer>();
  if (this->isFabric()) {
    this->expectThrow(fibContainer);
  } else {
    this->expectSwitchId(fibContainer);
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, labelFibEntry) {
  auto entry = std::make_shared<LabelForwardingEntry>();
  if (this->isFabric()) {
    this->expectThrow(entry);
  } else {
    this->expectSwitchId(entry);
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, bufferPoolCfgScope) {
  if (this->isFabric()) {
    this->expectThrow(cfg::BufferPoolConfig{});
    this->expectThrow(std::shared_ptr<BufferPoolCfg>{});
  } else {
    this->expectL3(cfg::BufferPoolConfig{});
    this->expectL3(std::shared_ptr<BufferPoolCfg>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, aclScope) {
  if (this->isFabric()) {
    this->expectThrow(cfg::AclEntry{});
    this->expectThrow(std::shared_ptr<AclEntry>{});
  } else {
    this->expectL3(cfg::AclEntry{});
    this->expectL3(std::shared_ptr<AclEntry>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, qosPolicyScope) {
  if (this->isFabric()) {
    this->expectThrow(cfg::QosPolicy{});
    this->expectThrow(std::shared_ptr<QosPolicy>{});
  } else {
    this->expectL3(cfg::QosPolicy{});
    this->expectL3(std::shared_ptr<QosPolicy>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, controlPlaneScope) {
  if (this->isFabric()) {
    this->expectThrow(std::shared_ptr<ControlPlane>{});
  } else {
    this->expectL3(std::shared_ptr<ControlPlane>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, sflowCollectors) {
  if (this->isFabric()) {
    this->expectThrow(cfg::SflowCollector{});
    this->expectThrow(std::shared_ptr<SflowCollector>{});
  } else {
    this->expectL3(cfg::SflowCollector{});
    this->expectL3(std::shared_ptr<SflowCollector>());
  }
}

TYPED_TEST(SwitchIdScopeResolverTest, switchSettingsScope) {
  this->expectAll(std::shared_ptr<SwitchSettings>());
}

TYPED_TEST(SwitchIdScopeResolverTest, portFlowletCfgScope) {
  if (this->isFabric()) {
    this->expectThrow(cfg::PortFlowletConfig{});
    this->expectThrow(std::shared_ptr<PortFlowletCfg>{});
  } else {
    this->expectL3(cfg::PortFlowletConfig{});
    this->expectL3(std::shared_ptr<PortFlowletCfg>());
  }
}
