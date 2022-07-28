#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "RackmonPlsManager.h"

using namespace rackmonsvc;
using nlohmann::json;

TEST(GpioLineTest, EmptyGpioChip) {
  GpioLine gLine = {
      .gpioChip = "",
      .offset = 1,
      .name = "testGpio",
  };
  EXPECT_THROW(gLine.open("rackmonTest"), std::invalid_argument);
}

TEST(GpioLineTest, GpioChipNotExist) {
  GpioLine gLine = {
      .gpioChip = "/dev/gpiochip0006",
      .offset = 1,
      .name = "testGpio",
  };
  EXPECT_THROW(gLine.open("rackmonTest"), std::invalid_argument);
}

TEST(GpioLineTest, NegativeOffset) {
  GpioLine gLine = {
      .gpioChip = "/dev/zero",
      .offset = -5,
      .name = "testGpio",
  };
  EXPECT_THROW(gLine.open("rackmonTest"), std::invalid_argument);
}

TEST(GpioLineTest, FailToOpenGpio) {
  GpioLine gLine = {
      .gpioChip = "/dev/null",
      .offset = 1,
      .name = "testGpio",
  };
  EXPECT_THROW(gLine.open("rackmonTest"), std::runtime_error);
}

TEST(PlsManagerTest, EmptyGpioChip) {
  std::string mockPlsConfig = R"({
    "ports": [
        {
            "name": "port1",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "",
                    "offset": 4
                },
                {
                    "type": "redundancy",
                    "gpioChip": "",
                    "offset": 0
                }
            ]
        },
        {
            "name": "port2",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 5
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 1
                }
            ]
        },
        {
            "name": "port3",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 6
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 2
                }
            ]
        }
    ]
  })";
  json jConfig = json::parse(mockPlsConfig);
  RackmonPlsManager plsManager;

  EXPECT_THROW(plsManager.loadPlsConfig(jConfig), std::invalid_argument);
}

TEST(PlsManagerTest, GpioChipNotExist) {
  std::string mockPlsConfig = R"({
    "ports": [
        {
            "name": "port1",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/gpiochip001122",
                    "offset": 4
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/gpiochip98765432",
                    "offset": 0
                }
            ]
        },
        {
            "name": "port2",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 5
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 1
                }
            ]
        },
        {
            "name": "port3",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 6
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 2
                }
            ]
        }
    ]
  })";
  json jConfig = json::parse(mockPlsConfig);
  RackmonPlsManager plsManager;

  EXPECT_THROW(plsManager.loadPlsConfig(jConfig), std::invalid_argument);
}

TEST(PlsManagerTest, NegativeOffset) {
  std::string mockPlsConfig = R"({
    "ports": [
        {
            "name": "port1",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": -3
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": -5
                }
            ]
        },
        {
            "name": "port2",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 5
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 1
                }
            ]
        },
        {
            "name": "port3",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 6
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 2
                }
            ]
        }
    ]
  })";
  json jConfig = json::parse(mockPlsConfig);
  RackmonPlsManager plsManager;

  EXPECT_THROW(plsManager.loadPlsConfig(jConfig), std::invalid_argument);
}

TEST(PlsManagerTest, FailToOpenGpiochip) {
  std::string mockPlsConfig = R"({
    "ports": [
        {
            "name": "port1",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 4
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 0
                }
            ]
        },
        {
            "name": "port2",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 5
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 1
                }
            ]
        },
        {
            "name": "port3",
            "lines": [
                {
                    "type": "power",
                    "gpioChip": "/dev/zero",
                    "offset": 6
                },
                {
                    "type": "redundancy",
                    "gpioChip": "/dev/zero",
                    "offset": 2
                }
            ]
        }
    ]
  })";
  json jConfig = json::parse(mockPlsConfig);
  RackmonPlsManager plsManager;

  EXPECT_THROW(plsManager.loadPlsConfig(jConfig), std::runtime_error);
}
