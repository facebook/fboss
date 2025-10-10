#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Test executor implementation to orchestrate the execution of different tests
#


import concurrent.futures
import subprocess
from typing import Dict, List

from .config import SdkcastleSpec
from .enums import RunMode
from .test_runner import create_test_runner


class TestExecutor:
    """Executes tests based on configuration"""

    def __init__(self, config: SdkcastleSpec):
        self.config = config
        self.test_runner = create_test_runner(config)

    def execute(self) -> Dict[str, any]:
        """Main execution entry point"""
        print(f"Starting SDKCastle execution with run mode: {self.config.run_mode}")

        if self.config.run_mode == RunMode.LIST_TEST_RUNNER_CMDS_ONLY:
            return self._list_test_commands()

        if not self.config.test_specs:
            raise ValueError("No test specifications provided")

        if self.config.run_mode == RunMode.FULL_RUN:
            return self._execute_all_tests()

        raise ValueError(f"Unsupported run mode: {self.config.run_mode}")

    def _list_test_commands(self) -> Dict[str, any]:
        """List all test runner commands without executing them"""
        commands = self._generate_all_test_commands()

        print("\n=== Generated Test Runner Commands ===")
        for i, cmd in enumerate(commands, 1):
            print(f"\nCommand {i}:")
            print(" ".join(cmd))

        return {
            "status": "success",
            "mode": "list-commands-only",
            "total_commands": len(commands),
            "commands": commands,
        }

    def _execute_all_tests(self) -> Dict[str, any]:
        """Execute all tests in parallel using multithreading"""
        commands = self._generate_all_test_commands()

        if not commands:
            return {
                "status": "success",
                "mode": "full-run",
                "total_tests": 0,
                "results": [],
            }

        print(f"\nExecuting {len(commands)} test commands in parallel...")

        results = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
            future_to_cmd = {
                executor.submit(self._execute_command, cmd): cmd for cmd in commands
            }

            for future in concurrent.futures.as_completed(future_to_cmd):
                cmd = future_to_cmd[future]
                try:
                    result = future.result()
                    results.append(result)
                    print(
                        f"Completed: {' '.join(cmd[:5])}... - Status: {result['status']}"
                    )
                except Exception as exc:
                    error_result = {
                        "command": cmd,
                        "status": "error",
                        "error": str(exc),
                        "stdout": "",
                        "stderr": "",
                        "return_code": -1,
                    }
                    results.append(error_result)
                    print(f"Error executing {' '.join(cmd[:5])}...: {exc}")

        # Summary
        successful = sum(1 for r in results if r["status"] == "success")
        failed = len(results) - successful

        print("\n=== Execution Summary ===")
        print(f"Total tests: {len(results)}")
        print(f"Successful: {successful}")
        print(f"Failed: {failed}")

        return {
            "status": "success",
            "mode": "full-run",
            "total_tests": len(results),
            "successful": successful,
            "failed": failed,
            "results": results,
        }

    def _execute_command(self, cmd: List[str]) -> Dict[str, any]:
        """Execute a single command"""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=3600,  # 1 hour timeout
            )

            return {
                "command": cmd,
                "status": "success" if result.returncode == 0 else "failed",
                "stdout": result.stdout,
                "stderr": result.stderr,
                "return_code": result.returncode,
            }
        except subprocess.TimeoutExpired:
            return {
                "command": cmd,
                "status": "timeout",
                "stdout": "",
                "stderr": "Command timed out after 1 hour",
                "return_code": -1,
            }
        except Exception as e:
            return {
                "command": cmd,
                "status": "error",
                "stdout": "",
                "stderr": str(e),
                "return_code": -1,
            }

    def _generate_all_test_commands(self) -> List[List[str]]:
        """Generate all test runner commands based on configuration"""
        commands = []
        test_specs = self.config.test_specs

        if not test_specs:
            return commands

        commands.extend(self._generate_hw_test_commands(test_specs))
        commands.extend(self._generate_agent_test_commands(test_specs))
        commands.extend(self._generate_warmboot_test_commands(test_specs))
        commands.extend(self._generate_link_test_commands(test_specs))
        commands.extend(self._generate_config_test_commands(test_specs))
        commands.extend(self._generate_benchmark_test_commands(test_specs))

        return commands

    def _generate_hw_test_commands(self, test_specs) -> List[List[str]]:
        """Generate hardware test commands"""
        commands = []
        if test_specs.hw_tests:
            for hw_test in test_specs.hw_tests:
                if hw_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in hw_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_hw_test_commands(
                            hw_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_agent_test_commands(self, test_specs) -> List[List[str]]:
        """Generate agent test commands"""
        commands = []
        if test_specs.agent_tests:
            for agent_test in test_specs.agent_tests:
                if agent_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in agent_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_agent_test_commands(
                            agent_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_warmboot_test_commands(self, test_specs) -> List[List[str]]:
        """Generate warmboot test commands"""
        commands = []
        if test_specs.n_warmboot_tests:
            for warmboot_test in test_specs.n_warmboot_tests:
                if warmboot_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in warmboot_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_n_warmboot_test_commands(
                            warmboot_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_link_test_commands(self, test_specs) -> List[List[str]]:
        """Generate link test commands"""
        commands = []
        if test_specs.link_tests:
            for link_test in test_specs.link_tests:
                if link_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in link_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_link_test_commands(
                            link_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_config_test_commands(self, test_specs) -> List[List[str]]:
        """Generate config test commands"""
        commands = []
        if test_specs.config_tests:
            for config_test in test_specs.config_tests:
                if config_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in config_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_config_test_commands(
                            config_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_benchmark_test_commands(self, test_specs) -> List[List[str]]:
        """Generate benchmark test commands"""
        commands = []
        if test_specs.benchmark_tests:
            for benchmark_test in test_specs.benchmark_tests:
                if benchmark_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in benchmark_test.common_test_spec.asic_test_options.items():
                        test_commands = self.test_runner.build_benchmark_test_commands(
                            benchmark_test, asic_type, asic_options
                        )
                        commands.extend(test_commands)
        return commands
