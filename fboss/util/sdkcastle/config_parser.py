#
# Copyright 2004-present Facebook. All Rights Reserved.
#
# JSON configuration parser for sdkcastle
#
#

# pyre-unsafe

import json
import logging
from typing import Any, Callable, Dict, List, Optional, TypeVar

from .config import (
    AgentScaleTestsSpec,
    AgentTestsSpec,
    AsicTestOptions,
    BassetQuery,
    BenchmarkTestsSpec,
    CommonTestSpec,
    ConfigTestsSpec,
    HwTestsSpec,
    LinkTestsSpec,
    NWarmbootTestsSpec,
    SdkcastleSpec,
    TestSpec,
)
from .enums import (
    AsicType,
    BootType,
    BuildMode,
    Hardware,
    MultiStage,
    NpuMode,
    RunMode,
    string_to_enum,
    TestRunnerMode,
)

T = TypeVar("T")


class ConfigParser:
    """Parser for SDKCastle configuration files"""

    def __init__(self):
        self.logger = logging.getLogger("sdkcastle.config_parser")

    def parse_from_file(self, file_path: str) -> SdkcastleSpec:
        """Parse configuration from JSON file"""
        self.logger.debug(f"Parsing configuration from file: {file_path}")
        with open(file_path, "r") as f:
            data = json.load(f)
        return self._parse_sdkcastle_spec(data)

    def parse_from_dict(self, data: Dict[str, Any]) -> SdkcastleSpec:
        """Parse configuration from dictionary"""
        return self._parse_sdkcastle_spec(data)

    def _string_to_enum(self, value: Optional[str], enum_class) -> Optional[Any]:
        """Parse enum value"""
        if value is None:
            return None
        return string_to_enum(value, enum_class)

    def _parse_sdkcastle_spec(self, data: Dict[str, Any]) -> SdkcastleSpec:
        """Parse SdkcastleSpec from dictionary"""
        test_specs = None
        if "testSpecs" in data:
            test_specs = self._parse_test_spec(data["testSpecs"])

        return SdkcastleSpec(
            test_sdk_version=data["testSdkVersion"],
            prev_sdk_version=data.get("prevSdkVersion"),
            next_sdk_version=data.get("nextSdkVersion"),
            test_specs=test_specs,
            run_mode=self._string_to_enum(data.get("runMode"), RunMode),
            build_mode=self._string_to_enum(data.get("buildMode"), BuildMode),
            test_runner_mode=self._string_to_enum(
                data.get("testRunnerMode"), TestRunnerMode
            ),
            test_out_dir_path=data.get("testOutDirPath", "/tmp/"),
        )

    def _parse_test_spec(self, data: Dict[str, Any]) -> TestSpec:
        """Parse TestSpec from dictionary"""
        return TestSpec(
            hw_tests=self._parse_test_list(
                data.get("hwTests", []), self._parse_hw_tests_spec
            ),
            agent_tests=self._parse_test_list(
                data.get("agentTests", []), self._parse_agent_tests_spec
            ),
            agent_scale_tests=self._parse_test_list(
                data.get("agentScaleTests", []), self._parse_agent_scale_tests_spec
            ),
            n_warmboot_tests=self._parse_test_list(
                data.get("nWarmbootTests", []), self._parse_n_warmboot_tests_spec
            ),
            link_tests=self._parse_test_list(
                data.get("linkTests", []), self._parse_link_tests_spec
            ),
            config_tests=self._parse_test_list(
                data.get("configTests", []), self._parse_config_tests_spec
            ),
            benchmark_tests=self._parse_test_list(
                data.get("benchmarkTests", []), self._parse_benchmark_tests_spec
            ),
        )

    def _parse_test_list(
        self, data: List[Dict[str, Any]], parser_func: Callable[[Dict[str, Any]], T]
    ) -> List[T]:
        """Parse list of test specifications"""
        return [parser_func(item) for item in data]

    def _parse_hw_tests_spec(self, data: Dict[str, Any]) -> HwTestsSpec:
        """Parse HwTestsSpec from dictionary"""
        return HwTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
        )

    def _parse_agent_tests_spec(self, data: Dict[str, Any]) -> AgentTestsSpec:
        """Parse AgentTestsSpec from dictionary"""
        return AgentTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
            npu_mode=self._string_to_enum(data.get("npuMode"), NpuMode),
            multi_stage=self._string_to_enum(data.get("multiStage"), MultiStage),
        )

    def _parse_agent_scale_tests_spec(
        self, data: Dict[str, Any]
    ) -> AgentScaleTestsSpec:
        """Parse AgentScaleTestsSpec from dictionary"""
        return AgentScaleTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
            npu_mode=self._string_to_enum(data.get("npuMode"), NpuMode),
            multi_stage=self._string_to_enum(data.get("multiStage"), MultiStage),
        )

    def _parse_n_warmboot_tests_spec(self, data: Dict[str, Any]) -> NWarmbootTestsSpec:
        """Parse NWarmbootTestsSpec from dictionary"""
        return NWarmbootTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
            npu_mode=self._string_to_enum(data.get("npuMode"), NpuMode),
            num_iterations=data.get("numIterations"),
        )

    def _parse_link_tests_spec(self, data: Dict[str, Any]) -> LinkTestsSpec:
        """Parse LinkTestsSpec from dictionary"""
        return LinkTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
        )

    def _parse_config_tests_spec(self, data: Dict[str, Any]) -> ConfigTestsSpec:
        """Parse ConfigTestsSpec from dictionary"""
        return ConfigTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
        )

    def _parse_benchmark_tests_spec(self, data: Dict[str, Any]) -> BenchmarkTestsSpec:
        """Parse BenchmarkTestsSpec from dictionary"""
        return BenchmarkTestsSpec(
            test_name=data["testName"],
            common_test_spec=self._parse_common_test_spec(data["commonTestSpec"]),
            npu_mode=self._string_to_enum(data.get("npuMode"), NpuMode),
        )

    def _parse_common_test_spec(self, data: Dict[str, Any]) -> CommonTestSpec:
        """Parse CommonTestSpec from dictionary"""
        asic_test_options = None
        if "asicTestOptions" in data:
            asic_test_options = {}
            for asic_str, options_data in data["asicTestOptions"].items():
                asic_enum = self._string_to_enum(asic_str, AsicType)
                asic_test_options[asic_enum] = self._parse_asic_test_options(
                    options_data
                )

        return CommonTestSpec(
            test_team=data["testTeam"],
            asic_test_options=asic_test_options,
            test_round_trip=data.get("test_round_trip", False),
        )

    def _parse_asic_test_options(self, data: Dict[str, Any]) -> AsicTestOptions:
        """Parse AsicTestOptions from dictionary"""
        return AsicTestOptions(
            asic_test_name=data["asicTestName"],
            basset_query=self._parse_basset_query(data["bassetQuery"]),
            build_mode=self._string_to_enum(data.get("buildMode"), BuildMode),
            boot_type=self._string_to_enum(data.get("bootType"), BootType),
            regex=data.get("regex"),
            num_jobs=data.get("numJobs", 4),
            runner_options=data.get("runnerOptions"),
        )

    def _parse_basset_query(self, data: Dict[str, Any]) -> BassetQuery:
        """Parse BassetQuery from dictionary"""
        return BassetQuery(
            device_pool=data["devicePool"],
            asic=self._string_to_enum(data.get("asic"), AsicType),
            hardware=self._string_to_enum(data.get("hardware"), Hardware),
            device=data.get("device"),
        )
