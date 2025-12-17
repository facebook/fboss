#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Test runner implementation for sdkcastle
#

# pyre-unsafe

from abc import ABC, abstractmethod
from typing import List, Optional, Tuple, Union

from pyre_extensions import none_throws

from .config import (
    AgentScaleTestsSpec,
    AgentTestsSpec,
    AsicTestOptions,
    BenchmarkTestsSpec,
    ConfigTestsSpec,
    HwTestsSpec,
    LinkTestsSpec,
    NWarmbootTestsSpec,
    SdkcastleSpec,
)
from .constants import (
    BENCHMARK_ASIC_CONFIG,
    BRCM_DNX_ASICS,
    J3AI_REV_NOT_A0,
    TEST_TYPE_TEAM_MAPPING,
)
from .enums import AsicType, TestRunnerMode


class BaseTestRunner(ABC):
    """Base class for test runner command generation"""

    def __init__(self, config: SdkcastleSpec):
        self.config = config

    @abstractmethod
    def build_hw_test_commands(
        self, hw_test: HwTestsSpec, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[Tuple[List[str], str]]:
        """Build hardware test commands"""
        pass

    @abstractmethod
    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent test commands"""
        pass

    @abstractmethod
    def build_agent_scale_test_commands(
        self,
        agent_scale_test: AgentScaleTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent scale test commands"""
        pass

    @abstractmethod
    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build link test commands"""
        pass

    @abstractmethod
    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build n-warmboot test commands"""
        pass

    @abstractmethod
    def build_config_test_commands(
        self,
        config_test: ConfigTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build config test commands"""
        pass

    @abstractmethod
    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build benchmark test commands"""
        pass


class NetcastleTestRunner(BaseTestRunner):
    """Test runner for meta-internal mode using netcastle commands"""

    def _build_netcastle_command(
        self,
        test_type: str,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
        agent_test: Optional[Union[AgentTestsSpec, AgentScaleTestsSpec]] = None,  # type: ignore
        n_warmboot_test: Optional[NWarmbootTestsSpec] = None,  # type: ignore
        benchmark_test: Optional[BenchmarkTestsSpec] = None,  # type: ignore
    ) -> Tuple[List[str], str, str, Optional[str], Optional[str]]:
        """Build netcastle command and return command, asic, sdk_project_version, npu_mode, multi_stage"""
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

        # Get NPU mode
        npu_mode = None
        if agent_test and hasattr(agent_test, "npu_mode") and agent_test.npu_mode:
            npu_mode = none_throws(agent_test.npu_mode).value

        if (
            n_warmboot_test
            and hasattr(n_warmboot_test, "npu_mode")
            and n_warmboot_test.npu_mode
        ):
            npu_mode = none_throws(n_warmboot_test.npu_mode).value

        if (
            benchmark_test
            and hasattr(benchmark_test, "npu_mode")
            and benchmark_test.npu_mode
        ):
            npu_mode = none_throws(benchmark_test.npu_mode).value

        # Get multi_stage
        multi_stage = None
        if agent_test and hasattr(agent_test, "multi_stage") and agent_test.multi_stage:
            multi_stage = none_throws(agent_test.multi_stage).value

        # Get ASIC and build ASIC string
        asic = asic_type.value
        asic_str = asic

        # Build SDK version string for the test config
        if benchmark_test:
            sdk_version_str = sdk_project_version
            if asic in BENCHMARK_ASIC_CONFIG:
                asic_str = BENCHMARK_ASIC_CONFIG[asic]
        else:
            sdk_version_str = f"{sdk_project_version}/{sdk_project_version}"

        # Build test-config: <vendor>/<sdkProjectVersion>/<sdkProjectVersion>/<asic>
        if npu_mode is not None:
            test_config = f"{vendor}/{sdk_version_str}/{asic_str}/{npu_mode}"
        else:
            test_config = f"{vendor}/{sdk_version_str}/{asic_str}"

        # Build ASIC string
        asic_str = asic
        if asic_str == "jericho3" and (benchmark_test or multi_stage):
            asic_str = f"{asic_str},{J3AI_REV_NOT_A0}"

        # Build basset-query: <devicePool>/asic=<asic>
        basset_query = f"{asic_options.basset_query.device_pool}/asic={asic_str}"

        num_jobs = asic_options.num_jobs

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

        multi_stage = None
        if agent_test and hasattr(agent_test, "multi_stage") and agent_test.multi_stage:
            multi_stage = none_throws(agent_test.multi_stage).value
            cmd.extend(["--multistage-role", multi_stage])

        if n_warmboot_test:
            num_iterations = "128"
            if (
                hasattr(n_warmboot_test, "num_iterations")
                and n_warmboot_test.num_iterations
            ):
                num_iterations = n_warmboot_test.num_iterations
            cmd.extend(["--num-wb-iterations", num_iterations])

        # Add regex if specified
        if asic_options.regex:
            cmd.extend(["--regex", asic_options.regex])

        return cmd, asic_type.value, sdk_project_version, npu_mode, multi_stage

    def _generate_log_filename(
        self,
        test_type: str,
        asic: str,
        sdk_project_version: str,
        npu_mode: Optional[str],
        multi_stage: Optional[str] = None,
    ) -> str:
        """Generate log filename based on test type and metadata"""
        if test_type == "agent":
            return (
                f"{asic}_{sdk_project_version}_agent_{npu_mode}.log"
                if multi_stage is None
                else f"{asic}_{sdk_project_version}_agent_{npu_mode}_{multi_stage}.log"
            )
        elif test_type == "agent-scale":
            return (
                f"{asic}_{sdk_project_version}_agent_scale_{npu_mode}.log"
                if multi_stage is None
                else f"{asic}_{sdk_project_version}_agent_scale_{npu_mode}_{multi_stage}.log"
            )
        elif test_type == "n-warmboot":
            return f"{asic}_{sdk_project_version}_n_wb.log"
        elif test_type == "link":
            return f"{asic}_{sdk_project_version}_link.log"
        elif test_type == "config":
            return f"{asic}_{sdk_project_version}_config.log"
        elif test_type == "benchmark":
            return f"{asic}_{sdk_project_version}_bench_{npu_mode}.log"
        else:
            return f"{asic}_{sdk_project_version}_hw.log"

    def build_hw_test_commands(
        self, hw_test: HwTestsSpec, asic_type: AsicType, asic_options: AsicTestOptions
    ) -> List[Tuple[List[str], str]]:
        """Build hardware test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command("hw", asic_type, asic_options)
        )
        log_filename = self._generate_log_filename(
            "hw", asic, sdk_project_version, npu_mode
        )
        return [(cmd, log_filename)]

    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command(
                "agent", asic_type, asic_options, agent_test=agent_test
            )
        )
        log_filename = self._generate_log_filename(
            "agent", asic, sdk_project_version, npu_mode, multi_stage
        )
        return [(cmd, log_filename)]

    def build_agent_scale_test_commands(
        self,
        agent_scale_test: AgentScaleTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent scale test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command(
                "agent-scale", asic_type, asic_options, agent_test=agent_scale_test
            )
        )
        log_filename = self._generate_log_filename(
            "agent-scale", asic, sdk_project_version, npu_mode, multi_stage
        )
        return [(cmd, log_filename)]

    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build link test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command("link", asic_type, asic_options)
        )
        log_filename = self._generate_log_filename(
            "link", asic, sdk_project_version, npu_mode
        )
        return [(cmd, log_filename)]

    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build n-warmboot test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command(
                "n-warmboot", asic_type, asic_options, n_warmboot_test=warmboot_test
            )
        )
        log_filename = self._generate_log_filename(
            "n-warmboot", asic, sdk_project_version, npu_mode, multi_stage
        )
        return [(cmd, log_filename)]

    def build_config_test_commands(
        self,
        config_test: ConfigTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build config test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command("config", asic_type, asic_options)
        )
        log_filename = self._generate_log_filename(
            "config", asic, sdk_project_version, npu_mode
        )
        return [(cmd, log_filename)]

    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build benchmark test commands"""
        cmd, asic, sdk_project_version, npu_mode, multi_stage = (
            self._build_netcastle_command(
                "benchmark", asic_type, asic_options, benchmark_test=benchmark_test
            )
        )
        log_filename = self._generate_log_filename(
            "benchmark", asic, sdk_project_version, npu_mode, multi_stage
        )
        return [(cmd, log_filename)]


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
    ) -> List[Tuple[List[str], str]]:
        """Build hardware test commands"""
        cmd = self._build_oss_command(hw_test.common_test_spec.test_team, asic_options)
        cmd.extend(["--test-type", "hw"])
        log_filename = f"{asic_type.value}_hw.log"
        return [(cmd, log_filename)]

    def build_agent_test_commands(
        self,
        agent_test: AgentTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent test commands"""
        cmd = self._build_oss_command(
            agent_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "agent"])

        npu_mode = "mono"
        if hasattr(agent_test, "npu_mode") and agent_test.npu_mode:
            npu_mode_value = agent_test.npu_mode.value
            cmd.extend(["--npu-mode", npu_mode_value])
            npu_mode = npu_mode_value

        if hasattr(agent_test, "multi_stage") and agent_test.multi_stage:
            cmd.extend(["--multi-stage", agent_test.multi_stage.value])

        log_filename = f"{asic_type.value}_agent_{npu_mode}.log"
        return [(cmd, log_filename)]

    def build_agent_scale_test_commands(
        self,
        agent_scale_test: AgentScaleTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build agent scale test commands"""
        cmd = self._build_oss_command(
            agent_scale_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "agent-scale"])

        npu_mode = "mono"
        if hasattr(agent_scale_test, "npu_mode") and agent_scale_test.npu_mode:
            npu_mode_value = agent_scale_test.npu_mode.value
            cmd.extend(["--npu-mode", npu_mode_value])
            npu_mode = npu_mode_value

        if hasattr(agent_scale_test, "multi_stage") and agent_scale_test.multi_stage:
            cmd.extend(["--multi-stage", agent_scale_test.multi_stage.value])

        log_filename = f"{asic_type.value}_agent_scale_{npu_mode}.log"
        return [(cmd, log_filename)]

    def build_link_test_commands(
        self,
        link_test: LinkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build link test commands"""
        cmd = self._build_oss_command(
            link_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "link"])
        log_filename = f"{asic_type.value}_link.log"
        return [(cmd, log_filename)]

    def build_n_warmboot_test_commands(
        self,
        warmboot_test: NWarmbootTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build n-warmboot test commands"""
        cmd = self._build_oss_command(
            warmboot_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "n-warmboot"])
        log_filename = f"{asic_type.value}_n_wb.log"
        return [(cmd, log_filename)]

    def build_config_test_commands(
        self,
        config_test: ConfigTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build config test commands"""
        cmd = self._build_oss_command(
            config_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "config"])
        log_filename = f"{asic_type.value}_config.log"
        return [(cmd, log_filename)]

    def build_benchmark_test_commands(
        self,
        benchmark_test: BenchmarkTestsSpec,
        asic_type: AsicType,
        asic_options: AsicTestOptions,
    ) -> List[Tuple[List[str], str]]:
        """Build benchmark test commands"""
        cmd = self._build_oss_command(
            benchmark_test.common_test_spec.test_team, asic_options
        )
        cmd.extend(["--test-type", "benchmark"])
        log_filename = f"{asic_type.value}_bench.log"
        return [(cmd, log_filename)]


def create_test_runner(config: SdkcastleSpec) -> BaseTestRunner:
    """Factory function to create the appropriate test runner based on config"""
    if config.test_runner_mode == TestRunnerMode.META_INTERNAL:
        return NetcastleTestRunner(config)
    else:
        return OSSTestRunner(config)
