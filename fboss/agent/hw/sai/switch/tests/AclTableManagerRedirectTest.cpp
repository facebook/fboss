// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

class AclTableManagerRedirectTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0_ = testInterfaces[0];
  }

  int kPriority() {
    return 1;
  }

  uint8_t kDscp() {
    return 10;
  }

  std::shared_ptr<IpTunnel> makeEncapTunnel(
      const std::string& tunnelId = "encapTunnel0") {
    auto tunnel = std::make_shared<IpTunnel>(tunnelId);
    tunnel->setType(TunnelType::IP_IN_IP_ENCAP);
    tunnel->setUnderlayIntfId(InterfaceID(intf0_.id));
    tunnel->setTTLMode(cfg::TunnelMode::PIPE);
    tunnel->setDscpMode(cfg::TunnelMode::PIPE);
    tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
    tunnel->setTunnelTermType(cfg::TunnelTerminationType::P2P);
    tunnel->setSrcIP(folly::IPAddressV6("2401:db00::1"));
    tunnel->setDstIP(folly::IPAddressV6("2401:db00:ffff::1"));
    return tunnel;
  }

  TestInterface intf0_;
};

TEST_F(AclTableManagerRedirectTest, addAclEntryWithTunnelEncapRedirect) {
  auto swTunnel = makeEncapTunnel("encapTunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);

  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());

  cfg::RedirectNextHop redirectNh;
  redirectNh.ip() = "2401:db00:ffff::1";
  redirectNh.tunnelType() = TunnelType::IP_IN_IP_ENCAP;
  redirectNh.tunnelId() = "encapTunnel0";

  cfg::RedirectToNextHopAction redirectAction;
  redirectAction.redirectNextHops() = {redirectNh};

  MatchAction matchAction;
  matchAction.setRedirectToNextHop(
      std::make_pair(redirectAction, MatchAction::NextHopSet()));
  aclEntry->setAclAction(matchAction);

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  ASSERT_TRUE(aclEntryHandle);
  ASSERT_TRUE(aclEntryHandle->tunnelEncapNextHop);

  auto redirectOid =
      saiApiTable->aclApi()
          .getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::ActionRedirect())
          .getData();
  EXPECT_EQ(redirectOid, aclEntryHandle->tunnelEncapNextHop->adapterKey());
}

TEST_F(AclTableManagerRedirectTest, removeAclEntryWithTunnelEncapRedirect) {
  auto swTunnel = makeEncapTunnel("encapTunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);

  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());

  cfg::RedirectNextHop redirectNh;
  redirectNh.ip() = "2401:db00:ffff::1";
  redirectNh.tunnelType() = TunnelType::IP_IN_IP_ENCAP;
  redirectNh.tunnelId() = "encapTunnel0";

  cfg::RedirectToNextHopAction redirectAction;
  redirectAction.redirectNextHops() = {redirectNh};

  MatchAction matchAction;
  matchAction.setRedirectToNextHop(
      std::make_pair(redirectAction, MatchAction::NextHopSet()));
  aclEntry->setAclAction(matchAction);

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  auto& nhStore = saiStore->get<SaiTunnelEncapNextHopTraits>();
  EXPECT_EQ(nhStore.size(), 1);

  saiManagerTable->aclTableManager().removeAclEntry(
      aclEntry, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  EXPECT_FALSE(aclEntryHandle);
  EXPECT_EQ(nhStore.size(), 0);
}

TEST_F(AclTableManagerRedirectTest, aclEntryRedirectMissingTunnel) {
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());

  cfg::RedirectNextHop redirectNh;
  redirectNh.ip() = "2401:db00:ffff::1";
  redirectNh.tunnelType() = TunnelType::IP_IN_IP_ENCAP;
  redirectNh.tunnelId() = "nonExistentTunnel";

  cfg::RedirectToNextHopAction redirectAction;
  redirectAction.redirectNextHops() = {redirectNh};

  MatchAction matchAction;
  matchAction.setRedirectToNextHop(
      std::make_pair(redirectAction, MatchAction::NextHopSet()));
  aclEntry->setAclAction(matchAction);

  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          aclEntry, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE()),
      FbossError);
}
