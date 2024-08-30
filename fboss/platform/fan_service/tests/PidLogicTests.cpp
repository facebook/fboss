// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/platform/fan_service/PidLogic.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

using namespace ::testing;
using namespace facebook::fboss::platform::fan_service;

TEST(PIDLogicTest, Basic) {
  PidSetting pidSetting;
  pidSetting.kp() = -3;
  pidSetting.ki() = -0.02;
  pidSetting.kd() = 0;
  pidSetting.setPoint() = 60;
  pidSetting.negHysteresis() = 2;
  pidSetting.posHysteresis() = 0;

  // CASE 1: The read value is lower than setPoint, and the previous target PWM
  //         is too high. PWM should go down. (60 -> 0)
  auto pidLogic1 = PidLogic(pidSetting, 5);
  pidLogic1.updateLastPwm(60);
  EXPECT_EQ(pidLogic1.calculatePwm(50), 0);
  EXPECT_EQ(pidLogic1.getLastError(), 8);

  // CASE 2: The read value is within desired range.
  //         PWM should not change. (30 -> 30)
  auto pidLogic2 = PidLogic(pidSetting, 5);
  pidLogic2.updateLastPwm(30);
  EXPECT_EQ(pidLogic2.calculatePwm(59), 30);
  EXPECT_EQ(pidLogic2.getLastError(), 0);

  // CASE 3: The read value is higher than setPoint, and the current target PWM
  //         is too low. PWM should go up a bit. (30 -> 24)
  auto pidLogic3 = PidLogic(pidSetting, 5);
  pidLogic3.updateLastPwm(30);
  EXPECT_EQ(pidLogic3.calculatePwm(68), 24);
  EXPECT_EQ(pidLogic3.getLastError(), -8);

  // CASE 4: Though read value is higher than setPoint, the current target PWM
  //         is overly high. PWM should go down. (99 -> 24)
  auto pidLogic4 = PidLogic(pidSetting, 5);
  pidLogic4.updateLastPwm(99);
  EXPECT_EQ(pidLogic4.calculatePwm(68), 24);
  EXPECT_EQ(pidLogic4.getLastError(), -8);
}

TEST(PIDLogicTest, Convergence) {
  PidSetting pidSetting;
  pidSetting.kp() = -0.05;
  pidSetting.ki() = -0.01;
  pidSetting.kd() = 0;
  pidSetting.setPoint() = 70;
  pidSetting.negHysteresis() = 2;
  pidSetting.posHysteresis() = 0;
  auto pidLogic = PidLogic(pidSetting, 30);
  pidLogic.updateLastPwm(40);
  double measurement = 80;
  int max_steps = 50;
  bool setpointReached = false;
  for (int i = 0; i < max_steps; ++i) {
    auto lastPwm = pidLogic.getLastPwm();
    auto newPwm = pidLogic.calculatePwm(measurement);
    measurement = measurement - (newPwm - lastPwm); // Simulate system response
    XLOG(DBG1) << fmt::format(
        "Iteration {}: Measurement = {}, newPwm = {}, Error = {}",
        i,
        measurement,
        newPwm,
        *pidSetting.setPoint() - measurement);
    if (*pidSetting.setPoint() - *pidSetting.negHysteresis() <= measurement &&
        measurement <= *pidSetting.setPoint() + *pidSetting.posHysteresis()) {
      setpointReached = true;
      break;
    }
  }
  EXPECT_TRUE(setpointReached);
}
