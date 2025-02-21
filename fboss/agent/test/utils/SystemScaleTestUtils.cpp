#include "fboss/agent/test/utils/SystemScaleTestUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/AclScaleTestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"

namespace facebook::fboss::utility {

void configureMaxAclEntries(AgentEnsemble* ensemble) {
  auto cfg = ensemble->getCurrentConfig();
  const auto maxAclEntries = getMaxAclEntries(ensemble->getL3Asics());
  XLOG(DBG2) << "Max Acl Entries: " << maxAclEntries;
  std::vector<cfg::AclTableQualifier> qualifiers =
      setAclQualifiers(AclWidth::SINGLE_WIDE);

  utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
  addAclEntries(maxAclEntries, AclWidth::SINGLE_WIDE, &cfg, "aclTable0");

  ensemble->applyNewConfig(cfg);
}

cfg::SwitchConfig getSystemScaleTestSwitchConfiguration(
    const AgentEnsemble& ensemble) {
  FLAGS_sai_user_defined_trap = true;
  auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
  auto asic = utility::checkSameAndGetAsic(l3Asics);
  auto config = utility::oneL3IntfNPortConfig(
      ensemble.getSw()->getPlatformMapping(),
      asic,
      ensemble.masterLogicalPortIds(),
      ensemble.getSw()->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes());

  utility::addAclTableGroup(
      &config, cfg::AclStage::INGRESS, utility::kDefaultAclTableGroupName());
  utility::addDefaultAclTable(config);
  // We don't want to set queue rate that limits the number of rx pkts
  utility::addCpuQueueConfig(
      config,
      ensemble.getL3Asics(),
      ensemble.isSai(),
      /* setQueueRate */ false);
  utility::setDefaultCpuTrafficPolicyConfig(
      config, ensemble.getL3Asics(), ensemble.isSai());
  return config;
};

void initSystemScaleTest(AgentEnsemble* ensemble) {
  configureMaxAclEntries(ensemble);
}
} // namespace facebook::fboss::utility
