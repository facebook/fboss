// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

// Credo SDK version comparison macro.
//
// Instead of checking individual SDK version flags everywhere:
//   #ifdef CREDO_SDK_0_9_0
//   #ifndef CREDO_SDK_0_9_0
//
// Use version comparisons that are future-proof:
//   #if CREDO_SDK_VERSION >= CREDO_SDK_VERSION_0_9_0
//   #if CREDO_SDK_VERSION < CREDO_SDK_VERSION_0_9_0
//
// When adding a new SDK version, just add a new #elif here.
// All existing >= / < comparisons will automatically include it.

#define CREDO_SDK_VERSION_0_7_2 702
#define CREDO_SDK_VERSION_0_9_0 900
#define CREDO_SDK_VERSION_1_2_7 10207
#define CREDO_SDK_VERSION_1_2_8 10208

#if defined(CREDO_SDK_1_2_7)
#define CREDO_SDK_VERSION CREDO_SDK_VERSION_1_2_7
#elif defined(CREDO_SDK_1_2_8)
#define CREDO_SDK_VERSION CREDO_SDK_VERSION_1_2_8
#elif defined(CREDO_SDK_0_9_0)
#define CREDO_SDK_VERSION CREDO_SDK_VERSION_0_9_0
#else
#define CREDO_SDK_VERSION CREDO_SDK_VERSION_0_7_2
#endif
