// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <sai.h>
#if defined(TAJO_SDK_GTE_26_5)
#include <saiextensions.h>
#elif __has_include(<experimental/sai_attr_ext.h>)
#include <experimental/sai_attr_ext.h>
#elif __has_include(<sai_attr_ext.h>)
#include <sai_attr_ext.h>
#endif
#if __has_include(<saicustom.h>)
#include <saicustom.h>
#define FBOSS_TAJO_SAI_CUSTOM_HEADERS 1
#endif
}
