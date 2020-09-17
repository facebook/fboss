/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"

#include <folly/logging/xlog.h>
#include <memory>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateToggler.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/platforms/tests/utils/CreateTestPlatform.h"

DECLARE_bool(setup_thrift);
DECLARE_int32(update_bststats_interval_s);

using apache::thrift::can_throw;

namespace {
void addPort(
    facebook::fboss::BcmConfig::ConfigMap& cfg,
    int port,
    int speed,
    bool enabled = true) {
  auto key = folly::to<std::string>("portmap_", port);
  auto value = folly::to<std::string>(port, ":", speed);
  if (!enabled) {
    value = folly::to<std::string>(value, ":i");
  }
  cfg[key] = value;
}

void addFlexPort(
    facebook::fboss::BcmConfig::ConfigMap& cfg,
    int start,
    int speed) {
  addPort(cfg, start, speed);
  addPort(cfg, start + 1, speed / 4, false);
  addPort(cfg, start + 2, speed / 2, false);
  addPort(cfg, start + 3, speed / 4, false);
}

void modifyCfgForQcmTests(facebook::fboss::BcmConfig::ConfigMap& cfg) {
  if (cfg.find("l2_mem_entries") != cfg.end()) {
    // remove
    cfg.erase("l2_mem_entries");
  }
  cfg["flowtracker_enable"] = "2";
  cfg["qcm_flow_enable"] = "1";
  cfg["qcm_max_flows"] = "1000";
  cfg["num_queues_pci"] = "46";
  cfg["num_queues_uc0"] = "1";
  cfg["num_queues_uc1"] = "1";
  cfg["flowtracker_max_export_pkt_length"] = "1500";
  cfg["flowtracker_enterprise_number"] = "0xAABBCCDD";
  cfg["fpem_mem_entries"] = "0x2000";
  cfg["ctr_evict_enable"] = "0";
}

} // namespace

namespace facebook::fboss {

BcmSwitchEnsemble::BcmSwitchEnsemble(
    const HwSwitchEnsemble::Features& featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {
  auto platform = createTestPlatform();
  std::unique_ptr<AgentConfig> agentConfig;
  BcmConfig::ConfigMap cfg;
  std::string yamlCfg;
  if (getPlatformMode() == PlatformMode::FAKE_WEDGE) {
    FLAGS_flexports = true;
    for (int n = 1; n <= 125; n += 4) {
      addFlexPort(cfg, n, 40);
    }
    cfg["pbmp_xport_xe"] = "0x1fffffffffffffffffffffffe";
    // Create an empty AgentConfig for now, once we fully roll out the new
    // platform config, we should generate a mock platform config for the fake
    // bcm test
    agentConfig = createEmptyAgentConfig();
  } else {
    // Load from a local file
    if (!FLAGS_bcm_config.empty()) {
      XLOG(INFO) << "Loading config from " << FLAGS_bcm_config;
      cfg = BcmConfig::loadFromFile(FLAGS_bcm_config);
      agentConfig = createEmptyAgentConfig();
    } else if (!FLAGS_config.empty()) {
      XLOG(INFO) << "Loading config from " << FLAGS_config;
      agentConfig = AgentConfig::fromFile(FLAGS_config);
      if (FLAGS_mode == "fuji") {
        yamlCfg = can_throw(*(platform->config()->thrift)
                                 .platform_ref()
                                 ->chip_ref()
                                 ->get_bcm()
                                 .yamlConfig_ref());
      } else {
        cfg = *(platform->config()->thrift)
                   .platform_ref()
                   ->chip_ref()
                   ->get_bcm()
                   .config_ref();
      }
    } else {
      throw FbossError("No config file to load!");
    }
    for (auto item : *agentConfig->thrift.defaultCommandLineArgs_ref()) {
      gflags::SetCommandLineOptionWithMode(
          item.first.c_str(), item.second.c_str(), gflags::SET_FLAG_IF_DEFAULT);
    }
  }
  if (FLAGS_load_qcm_fw &&
      platform->getAsic()->isSupported(HwAsic::Feature::QCM)) {
    XLOG(INFO) << "Modify bcm cfg as load_qcm_fw is enabled";
    modifyCfgForQcmTests(cfg);
  }
  if (FLAGS_mode == "fuji") {
    BcmAPI::initHSDK(yamlCfg);
  } else {
    BcmAPI::init(cfg);
  }
  // TODO pass agent config to platform init
  platform->init(std::move(agentConfig), getHwSwitchFeatures());
  auto bcmTestPlatform = static_cast<BcmTestPlatform*>(platform.get());
  std::unique_ptr<HwLinkStateToggler> linkToggler;
  if (haveFeature(HwSwitchEnsemble::LINKSCAN)) {
    linkToggler = createLinkToggler(
        static_cast<BcmSwitch*>(platform->getHwSwitch()),
        bcmTestPlatform->getAsic()->desiredLoopbackMode());
  }
  std::unique_ptr<std::thread> thriftThread;
  if (FLAGS_setup_thrift) {
    thriftThread =
        createThriftThread(static_cast<BcmSwitch*>(platform->getHwSwitch()));
  }
  setupEnsemble(
      std::move(platform), std::move(linkToggler), std::move(thriftThread));
  // Set bst stats update interval to 0 so we always refresh BST stats
  // in each updateStats call
  FLAGS_update_bststats_interval_s = 0;
  getPlatform()->initLEDs(getHwSwitch()->getUnit());
}

std::vector<PortID> BcmSwitchEnsemble::logicalPortIds() const {
  return getPlatform()->logicalPortIds();
}

std::vector<PortID> BcmSwitchEnsemble::masterLogicalPortIds() const {
  return getPlatform()->masterLogicalPortIds();
}

std::vector<PortID> BcmSwitchEnsemble::getAllPortsInGroup(PortID portID) const {
  return getPlatform()->getAllPortsInGroup(portID);
}

std::vector<FlexPortMode> BcmSwitchEnsemble::getSupportedFlexPortModes() const {
  return getPlatform()->getSupportedFlexPortModes();
}

void BcmSwitchEnsemble::dumpHwCounters() const {
  getHwSwitch()->printDiagCmd("show c");
}

std::map<PortID, HwPortStats> BcmSwitchEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  auto rv = bcm_stat_sync(getHwSwitch()->getUnit());
  bcmCheckError(rv, "Unable to sync stats ");
  updateHwSwitchStats(getHwSwitch());
  std::map<PortID, HwPortStats> mapPortStats;
  for (const auto& port : ports) {
    auto stats =
        getHwSwitch()->getPortTable()->getBcmPort(port)->getPortStats();
    mapPortStats[port] = (stats) ? *stats : HwPortStats{};
  }
  return mapPortStats;
}

bool BcmSwitchEnsemble::isRouteScaleEnabled() const {
  return getHwSwitch()->isAlpmEnabled();
}

std::unique_ptr<HwLinkStateToggler> BcmSwitchEnsemble::createLinkToggler(
    HwSwitch* hwSwitch,
    cfg::PortLoopbackMode desiredLoopbackMode) {
  return std::make_unique<BcmLinkStateToggler>(
      static_cast<BcmSwitch*>(hwSwitch),
      [this](const std::shared_ptr<SwitchState>& toApply) {
        applyNewState(toApply);
      },
      desiredLoopbackMode);
}

uint64_t BcmSwitchEnsemble::getSwitchId() const {
  return getHwSwitch()->getUnit();
}
} // namespace facebook::fboss
