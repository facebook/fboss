#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Test runner implementation for sdkcastle
#


from abc import ABC, abstractmethod
from typing import List

from .config import (
    AgentTestsSpec,
    AsicTestOptions,
    BenchmarkTestsSpec,
    HwTestsSpec,
    LinkTestsSpec,
    NWarmbootTestsSpec,
    SdkcastleSpec,
    SpecTestsSpec,
)
from .constants import BRCM_DNX_ASICS, TEST_TYPE_TEAM_MAPPING
from .enums import AsicType, TestRunnerMode


class BaseTestRunner(ABC):
    """Base class for test runner command generation"""

    def __init__(self, config: SdkcastleSpec):
        self.config = config

    @abstractmethod
    def build_hw_test_commands(
        self, hw_test: HwTestsSpec, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[List[str]]:
        """Build hardware test commands"""
        pass

    @abstractmethod
    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build agent test commands"""
        pass

    @abstractmethod
    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build link test commands"""
        pass

    @abstractmethod
    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build n-warmboot test commands"""
        pass

    @abstractmethod
    def build_config_test_commands(
        self,
        config_test: SpecTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build config test commands"""
        pass

    @abstractmethod
    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build benchmark test commands"""
        pass


class NetcastleTestRunner(BaseTestRunner):
    """Test runner for meta-internal mode using netcastle commands"""

    def _build_netcastle_command(
        self, test_type: str, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[str]:
        """Build netcastle command for meta-internal mode"""
        # Get team from mapping
        team = TEST_TYPE_TEAM_MAPPING.get(test_type, "sai_agent_test")

        # Parse SDK version to extract vendor and project version
        sdk_version = self.config.test_sdk_version

        # Determine vendor based on SDK version prefix
        if sdk_version.startswith("brcm"):
            vendor = "brcm"
        elif sdk_version.startswith("leaba"):
            vendor = "leaba"
        elif sdk_version.startswith("chenab") or sdk_version.startswith("nvda"):
            vendor = "nvda"
        else:
            # raise an exception if vendor is not recognized
            raise ValueError(f"Unknown SDK vendor in version: {sdk_version}")

        # Extract SDK project version (part after '/')
        if "/" in sdk_version:
            sdk_project_version = sdk_version.split("/", 1)[1]
        else:
            # Fallback to full version if no '/' found
            sdk_project_version = sdk_version

        # For BRCM vendor, replace "odp" with "dnx_odp" for DNX ASICs tests
        if vendor == "brcm" and asic_type.value in BRCM_DNX_ASICS:
            sdk_project_version = sdk_project_version.replace("odp", "dnx_odp")

        # TODO: Get the npu mode
        npu_mode = "mono"

        # Build test-config: <vendor>/<sdkProjectVersion>/<sdkProjectVersion>/<asic>
        test_config = f"{vendor}/{sdk_project_version}/{sdk_project_version}/{asic_type.value}/{npu_mode}"

        # Build basset-query: <devicePool>/asic=<asic>
        basset_query = f"{asic_options.basset_query.device_pool}/asic={asic_type.value}"

        # TODO: Get the num jobs
        num_jobs = 4

        build_mode_value = (
            asic_options.build_mode.value
            if asic_options.build_mode
            else (self.config.build_mode.value if self.config.build_mode else "opt")
        )

        # Build base netcastle command
        cmd = [
            "netcastle",
            "--team",
            team,
            "--test-config",
            test_config,
            "--buck-mode",
            build_mode_value,
            "--basset-query",
            basset_query,
            "--jobs",
            str(num_jobs),
        ]

        # Add regex if specified
        if asic_options.regex:
            cmd.extend(["--regex", asic_options.regex])

        return cmd

    def build_hw_test_commands(
        self, hw_test: HwTestsSpec, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[List[str]]:
        """Build hardware test commands"""
        cmd = self._build_netcastle_command("hw", asic_type, asic_options)
        return [cmd]

    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build agent test commands"""
        cmd = self._build_netcastle_command("agent", asic_type, asic_options)
        return [cmd]

    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build link test commands"""
        cmd = self._build_netcastle_command("link", asic_type, asic_options)
        return [cmd]

    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build n-warmboot test commands"""
        cmd = self._build_netcastle_command("n-warmboot", asic_type, asic_options)
        return [cmd]

    def build_config_test_commands(
        self,
        config_test: SpecTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build config test commands"""
        cmd = self._build_netcastle_command("config", asic_type, asic_options)
        return [cmd]

    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build benchmark test commands"""
        cmd = self._build_netcastle_command("benchmark", asic_type, asic_options)
        return [cmd]


class OSSTestRunner(BaseTestRunner):
    """Test runner for OSS mode using run_test.py commands"""

    def _build_oss_command(
        self, test_team: str, asic_options: AsicTestOptions
    ) -> List[str]:
        """Build oss command for OSS mode"""
        base_cmd = ["run_test.py"]

        base_cmd.extend(
            [
                "--",
                "--test-team",
                test_team,
                "--asic-test-name",
                asic_options.asic_test_name,
                "--device-pool",
                asic_options.basset_query.device_pool,
                "--num-jobs",
                str(asic_options.num_jobs),
            ]
        )

        if asic_options.basset_query.asic:
            base_cmd.extend(["--asic", asic_options.basset_query.asic.value])

        if asic_options.basset_query.hardware:
            base_cmd.extend(["--hardware", asic_options.basset_query.hardware.value])

        if asic_options.basset_query.device:
            base_cmd.extend(["--device", asic_options.basset_query.device])

        if asic_options.build_mode:
            base_cmd.extend(["--build-mode", asic_options.build_mode.value])

        if asic_options.boot_type:
            base_cmd.extend(["--boot-type", asic_options.boot_type.value])

        if asic_options.regex:
            base_cmd.extend(["--regex", asic_options.regex])

        if asic_options.runner_options:
            base_cmd.extend(["--runner-options", asic_options.runner_options])

        base_cmd.extend(["--test-out-dir-path", self.config.test_out_dir_path])

        return base_cmd

    def build_hw_test_commands(
        self, hw_test: HwTestsSpec, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[List[str]]:
        """Build hardware test commands"""
        cmd = self._build_oss_command(hw_test.common_test_spec.test_team, asic_options)
        cmd.extend(["--test-type", "hw"])
        return [cmd]

    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build agent test commands"""
        cmd = self._build_oss_command(
            agent_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "agent"])

        if hasattr(agent_test, "npu_mode") and agent_test.npu_mode:
            cmd.extend(["--npu-mode", agent_test.npu_mode.value])

        if hasattr(agent_test, "multi_stage") and agent_test.multi_stage:
            cmd.extend(["--multi-stage", agent_test.multi_stage.value])

        return [cmd]

    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build link test commands"""
        cmd = self._build_oss_command(
            link_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "link"])
        return [cmd]

    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build n-warmboot test commands"""
        cmd = self._build_oss_command(
            warmboot_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "n-warmboot"])
        return [cmd]

    def build_config_test_commands(
        self,
        config_test: SpecTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build config test commands"""
        cmd = self._build_oss_command(
            config_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "config"])
        return [cmd]

    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[List[str]]:
        """Build benchmark test commands"""
        cmd = self._build_oss_command(
            benchmark_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "benchmark"])
        return [cmd]


def create_test_runner(config: SdkcastleSpec) -> BaseTestRunner:
    """Factory function to create the appropriate test runner based on config"""
    if config.test_runner_mode == TestRunnerMode.META_INTERNAL:
        return NetcastleTestRunner(config)
    else:
        return OSSTestRunner(config)
