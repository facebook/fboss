#
# Copyright 2004-present Facebook. All Rights Reserved.
#
# Configuration data classes for sdkcastle based on thrift spec
#

# pyre-unsafe

from dataclasses import dataclass
from typing import Dict, List, Optional

from .enums import (
    AsicType,
    BootType,
    BuildMode,
    Hardware,
    MultiStage,
    NpuMode,
    RunMode,
    TestRunnerMode,
)


@dataclass
class BassetQuery:
    """Basset query configuration"""

    device_pool: str
    asic: Optional[AsicType] = None
    hardware: Optional[Hardware] = None
    device: Optional[str] = None


@dataclass
class AsicTestOptions:
    """ASIC test options"""

    asic_test_name: str
    basset_query: BassetQuery
    build_mode: Optional[BuildMode] = None
    boot_type: Optional[BootType] = None
    regex: Optional[str] = None
    num_jobs: int = 4
    runner_options: Optional[str] = None


@dataclass
class CommonTestSpec:
    """Common test specification"""

    test_team: str
    asic_test_options: Optional[Dict[AsicType, AsicTestOptions]] = None
    test_round_trip: bool = False


@dataclass
class HwTestsSpec:
    """Hardware tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec


@dataclass
class AgentTestsSpec:
    """Agent tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec
    npu_mode: Optional[NpuMode] = None
    multi_stage: Optional[MultiStage] = None


@dataclass
class AgentScaleTestsSpec:
    """Agent scale tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec
    npu_mode: Optional[NpuMode] = None
    multi_stage: Optional[MultiStage] = None


@dataclass
class NWarmbootTestsSpec:
    """N-warmboot tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec
    npu_mode: Optional[NpuMode] = None
    num_iterations: Optional[MultiStage] = None


@dataclass
class LinkTestsSpec:
    """Link tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec


@dataclass
class ConfigTestsSpec:
    """Config tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec


@dataclass
class BenchmarkTestsSpec:
    """Benchmark tests specification"""

    test_name: str
    common_test_spec: CommonTestSpec
    npu_mode: Optional[NpuMode] = None


@dataclass
class TestSpec:
    """Test specifications"""

    hw_tests: Optional[List[HwTestsSpec]] = None
    agent_tests: Optional[List[AgentTestsSpec]] = None
    agent_scale_tests: Optional[List[AgentScaleTestsSpec]] = None
    n_warmboot_tests: Optional[List[NWarmbootTestsSpec]] = None
    link_tests: Optional[List[LinkTestsSpec]] = None
    config_tests: Optional[List[ConfigTestsSpec]] = None
    benchmark_tests: Optional[List[BenchmarkTestsSpec]] = None


@dataclass
class SdkcastleSpec:
    """Main SDKCastle specification"""

    test_sdk_version: str
    prev_sdk_version: Optional[str] = None
    next_sdk_version: Optional[str] = None
    test_specs: Optional[TestSpec] = None
    run_mode: Optional[RunMode] = None
    build_mode: Optional[BuildMode] = None
    test_runner_mode: Optional[TestRunnerMode] = None
    test_out_dir_path: str = "/tmp/"
