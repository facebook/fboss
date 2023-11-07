// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/AgentDirectoryUtil.h"

using namespace ::testing;

using namespace facebook::fboss;

TEST(AgentDirectoryUtilTests, Verify) {
  AgentDirectoryUtil util;
  EXPECT_EQ(
      util.getSwColdBootOnceFile(),
      "/dev/shm/fboss/warm_boot/sw_cold_boot_once");
  EXPECT_EQ(
      util.getHwColdBootOnceFile(0),
      "/dev/shm/fboss/warm_boot/hw_cold_boot_once_0");
  EXPECT_EQ(
      util.getHwColdBootOnceFile(1),
      "/dev/shm/fboss/warm_boot/hw_cold_boot_once_1");
  EXPECT_EQ(
      util.getColdBootOnceFile(), "/dev/shm/fboss/warm_boot/cold_boot_once_0");

  EXPECT_EQ(
      util.getPackageDirectory(),
      "/etc/packages/neteng-fboss-wedge_agent/current");
  EXPECT_EQ(util.getSystemdDirectory(), "/etc/systemd/system");
  EXPECT_EQ(util.getConfigDirectory(), "/etc/coop/agent");
  EXPECT_EQ(util.getDrainConfigDirectory(), "/etc/coop/agent_drain");

  EXPECT_EQ(
      util.getPackageDirectory(),
      "/etc/packages/neteng-fboss-wedge_agent/current");
  EXPECT_EQ(
      util.getMultiSwitchScriptsDirectory(),
      util.getPackageDirectory() + "/multi_switch_agent_scripts");
  EXPECT_EQ(util.getSystemdDirectory(), "/etc/systemd/system");
  EXPECT_EQ(util.getConfigDirectory(), "/etc/coop/agent");
  EXPECT_EQ(util.getDrainConfigDirectory(), "/etc/coop/agent_drain");

  /* /etc/coop/agent/current */
  EXPECT_EQ(util.getAgentLiveConfig(), util.getConfigDirectory() + "/current");

  /* /etc/coop/agent_drain/current */
  EXPECT_EQ(
      util.getAgentDrainConfig(), util.getDrainConfigDirectory() + "/current");

  /* /etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/fboss_sw_agent.service
   */
  EXPECT_EQ(
      util.getSwAgentServicePath(),
      util.getMultiSwitchScriptsDirectory() + "/fboss_sw_agent.service");

  /* /etc/systemd/system/fboss_sw_agent.service */
  EXPECT_EQ(
      util.getSwAgentServiceSymLink(),
      util.getSystemdDirectory() + "/fboss_sw_agent.service");

  /* /etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/fboss_hw_agent@.service
   */
  EXPECT_EQ(
      util.getHwAgentServiceTemplatePath(),
      util.getMultiSwitchScriptsDirectory() + "/fboss_hw_agent@.service");

  EXPECT_EQ(util.getHwAgentServiceInstance(0), "fboss_hw_agent@0.service");

  /* /etc/systemd/system/fboss_hw_agent@.service */
  EXPECT_EQ(
      util.getHwAgentServiceTemplateSymLink(),
      util.getSystemdDirectory() + "/fboss_hw_agent@.service");

  /* /etc/systemd/system/fboss_hw_agent@0.service */
  EXPECT_EQ(
      util.getHwAgentServiceInstanceSymLink(0),
      util.getSystemdDirectory() + "/" + util.getHwAgentServiceInstance(0));

  /* /dev/shm/fboss/UNDRAINED */
  EXPECT_EQ(util.getUndrainedFlag(), util.getVolatileStateDir() + "/UNDRAINED");

  /* /dev/shm/fboss/agent_startup_config */
  EXPECT_EQ(
      util.getStartupConfig(),
      util.getVolatileStateDir() + "/agent_startup_config");

  /*
  /etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/pre_multi_switch_agent_start.par
  */
  EXPECT_EQ(
      util.getMultiSwitchPreStartScript(),
      util.getMultiSwitchScriptsDirectory() +
          "/pre_multi_switch_agent_start.par");

  /* /dev/shm/fboss/pre_start.sh */
  EXPECT_EQ(
      util.getPreStartShellScript(),
      util.getVolatileStateDir() + "/pre_start.sh");
}
