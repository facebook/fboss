// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <sai.h>

typedef enum _sai_port_serdes_extensions_attr_t {
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE =
      SAI_PORT_SERDES_ATTR_CUSTOM_RANGE_START,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSP_MODE,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS,
} sai_port_serdes_extensions_attr_t;

typedef enum _sai_switch_extensions_attr_t {
  SAI_SWITCH_ATTR_EXT_FAKE_LED = SAI_SWITCH_ATTR_END,
  SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET,
} sai_switch_extensions_attr_t;
