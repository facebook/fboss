/*
 * Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 facebook.fboss.sdkcastle
namespace py3 facebook.fboss.sdkcastle

enum RunMode {
  LIST_TEST_RUNNER_CMDS_ONLY = 0,
  FULL_RUN = 1,
}

enum BuildMode {
  OPT = 0,
  OPT_ASAN = 1,
  DBG = 2,
}

enum TestRunnerMode {
  META_INTERNAL = 0,
  OSS = 1,
}

enum AsicType {
  JERICHO3 = 0,
  RAMON3 = 1,
  TOMAHAWK4 = 2,
  TRIDENT4 = 3,
}

enum Hardware {
  WEDGE400 = 0,
  WEDGE400C = 1,
  MINIPACK2 = 2,
  YAMP = 3,
}

enum BootType {
  COLD_BOOT = 0,
  WARM_BOOT = 1,
}

enum NpuMode {
  MONO = 0,
  MULTI_SWITCH = 1,
}

enum MultiStage {
  ENABLED = 0,
  DISABLED = 1,
}

struct BassetQuery {
  1: string devicePool;
  2: optional AsicType asic;
  3: optional Hardware hardware;
  4: optional string device;
}

struct AsicTestOptions {
  1: string asicTestName;
  2: BassetQuery bassetQuery;
  3: optional BuildMode buildMode;
  4: optional BootType bootType;
  5: optional string regex;
  6: i32 numJobs = 4;
  7: optional string runnerOptions;
}

struct CommonTestSpec {
  1: string testTeam;
  2: optional map<AsicType, AsicTestOptions> asicTestOptions;
  3: bool testRoundTrip = false;
}

struct HwTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
}

struct AgentTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
  3: NpuMode npuMode = NpuMode.MONO;
  4: optional MultiStage multiStage;
}

struct AgentScaleTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
  3: NpuMode npuMode = NpuMode.MONO;
  4: optional MultiStage multiStage;
}

struct NWarmbootTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
  3: NpuMode npuMode = NpuMode.MONO;
  4: optional string numIterations;
}

struct LinkTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
}

struct ConfigTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
}

struct BenchmarkTestsSpec {
  1: string testName;
  2: CommonTestSpec commonTestSpec;
}

struct TestSpec {
  1: optional list<HwTestsSpec> hwTests;
  2: optional list<AgentTestsSpec> agentTests;
  3: optional list<NWarmbootTestsSpec> nWarmbootTests;
  4: optional list<LinkTestsSpec> linkTests;
  5: optional list<ConfigTestsSpec> configTests;
  6: optional list<BenchmarkTestsSpec> benchmarkTests;
  7: optional list<AgentScaleTestsSpec> agentScaleTests;
}

struct SdkcastleSpec {
  1: string testSdkVersion;
  2: optional string prevSdkVersion;
  3: optional string nextSdkVersion;
  4: optional TestSpec testSpecs;
  5: RunMode runMode = RunMode.FULL_RUN;
  6: optional BuildMode buildMode;
  7: optional TestRunnerMode testRunnerMode;
  8: string testOutDirPath = "/tmp/";
}
