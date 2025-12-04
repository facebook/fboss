#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Test executor implementation to orchestrate the execution of different tests
#

# pyre-unsafe

import concurrent.futures
import subprocess
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from .config import SdkcastleSpec
from .enums import RunMode
from .test_runner import create_test_runner
from .test_runner_report_generator import create_summary_report_generator


class TestExecutor:
    """Executes tests based on configuration"""

    def __init__(self, config: SdkcastleSpec):
        self.config = config
        self.test_runner = create_test_runner(config)
        self.log_dir: Optional[Path] = None

    def _create_log_directory(self) -> Path:
        """Create timestamped sdkcastle log directory under testOutDirPath"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_dir_name = f"sdkcastle_{timestamp}"

        base_path = Path(self.config.test_out_dir_path).expanduser()
        log_dir = base_path / log_dir_name

        log_dir.mkdir(parents=True, exist_ok=True)
        print(f"Created log directory: {log_dir}")

        return log_dir

    def _save_commands_to_file(
        self, commands: List[Tuple[List[str], str]], log_dir: Path
    ) -> None:
        """Save generated commands to netcastle_commands.txt"""
        commands_file = log_dir / "netcastle_commands.txt"

        with open(commands_file, "w") as f:
            for i, (cmd, log_filename) in enumerate(commands, 1):
                f.write(f"Command {i}:\n")
                f.write(" ".join(cmd) + "\n")
                f.write(f"Log file: {log_filename}\n\n")

        print(f"Saved commands to: {commands_file}")

    def execute(self) -> Dict[str, Any]:
        """Main execution entry point"""
        print(f"Starting SDKCastle execution with run mode: {self.config.run_mode}")

        if self.config.run_mode == RunMode.LIST_TEST_RUNNER_CMDS_ONLY:
            return self._list_test_commands()

        if not self.config.test_specs:
            raise ValueError("No test specifications provided")

        if self.config.run_mode == RunMode.FULL_RUN:
            return self._execute_all_tests()

        raise ValueError(f"Unsupported run mode: {self.config.run_mode}")

    def _list_test_commands(self) -> Dict[str, Any]:
        """List all test runner commands without executing them"""
        commands = self._generate_all_test_commands()

        self.log_dir = self._create_log_directory()
        self._save_commands_to_file(commands, self.log_dir)

        print("\n=== Generated Test Runner Commands ===")
        for i, (cmd, log_filename) in enumerate(commands, 1):
            print(f"\nCommand {i}:")
            print(" ".join(cmd))
            print(f"Log file: {log_filename}")

        return {
            "status": "success",
            "mode": "list-commands-only",
            "total_commands": len(commands),
            "commands": [cmd for cmd, _ in commands],
            "log_directory": str(self.log_dir),
        }

    def _execute_all_tests(self) -> Dict[str, Any]:
        """Execute all tests in parallel using multithreading"""
        commands = self._generate_all_test_commands()

        if not commands:
            return {
                "status": "success",
                "mode": "full-run",
                "total_tests": 0,
                "results": [],
            }

        self.log_dir = self._create_log_directory()
        self._save_commands_to_file(commands, self.log_dir)

        print(f"\nExecuting {len(commands)} test commands in parallel...")

        results = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=16) as executor:
            future_to_cmd = {
                executor.submit(self._execute_command, cmd, log_filename): (
                    cmd,
                    log_filename,
                )
                for cmd, log_filename in commands
            }

            for future in concurrent.futures.as_completed(future_to_cmd):
                cmd, log_filename = future_to_cmd[future]
                try:
                    result = future.result()
                    results.append(result)
                    print(
                        f"Completed: {' '.join(cmd)} ... - Status: {result['status']}\n"
                    )
                except Exception as exc:
                    error_result = {
                        "command": cmd,
                        "status": "error",
                        "error": str(exc),
                        "log_file": log_filename,
                        "return_code": -1,
                    }
                    results.append(error_result)
                    print(f"Error executing {' '.join(cmd)}...: {exc}")

        # Summary
        successful = sum(1 for r in results if r["status"] == "success")
        failed = len(results) - successful

        print("\n=== Execution Summary ===")
        print(f"Total tests: {len(results)}")
        print(f"Successful: {successful}")
        print(f"Failed: {failed}")
        print(f"Log directory: {self.log_dir}")

        # Generate test summary report
        self._generate_test_summary_report()

        return {
            "status": "success",
            "mode": "full-run",
            "total_tests": len(results),
            "successful": successful,
            "failed": failed,
            "results": results,
            "log_directory": str(self.log_dir),
        }

    def _execute_command(self, cmd: List[str], log_filename: str) -> Dict[str, Any]:
        """Execute a single command and redirect output to log file"""
        log_dir = self.log_dir
        if not log_dir:
            raise ValueError("Log directory not initialized")

        log_file_path = log_dir / log_filename

        try:
            with open(log_file_path, "w") as log_file:
                result = subprocess.run(
                    cmd,
                    stdout=log_file,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=3600 * 12,
                )

            return {
                "command": cmd,
                "status": "success" if result.returncode == 0 else "failed",
                "log_file": str(log_file_path),
                "return_code": result.returncode,
            }
        except subprocess.TimeoutExpired:
            with open(log_file_path, "a") as log_file:
                log_file.write("\n\nCommand timed out after 1 hour\n")
            return {
                "command": cmd,
                "status": "timeout",
                "log_file": str(log_file_path),
                "return_code": -1,
            }
        except Exception as e:
            with open(log_file_path, "a") as log_file:
                log_file.write(f"\n\nError executing command: {str(e)}\n")
            return {
                "command": cmd,
                "status": "error",
                "log_file": str(log_file_path),
                "return_code": -1,
            }

    def _generate_all_test_commands(self) -> List[Tuple[List[str], str]]:
        """Generate all test runner commands based on configuration"""
        commands = []
        test_specs = self.config.test_specs

        if not test_specs:
            return commands

        commands.extend(self._generate_hw_test_commands(test_specs))
        commands.extend(self._generate_agent_test_commands(test_specs))
        commands.extend(self._generate_agent_scale_test_commands(test_specs))
        commands.extend(self._generate_warmboot_test_commands(test_specs))
        commands.extend(self._generate_link_test_commands(test_specs))
        commands.extend(self._generate_config_test_commands(test_specs))
        commands.extend(self._generate_benchmark_test_commands(test_specs))

        return commands

    def _generate_hw_test_commands(self, test_specs) -> List[Tuple[List[str], str]]:
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

    def _generate_agent_test_commands(self, test_specs) -> List[Tuple[List[str], str]]:
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

    def _generate_agent_scale_test_commands(
        self, test_specs
    ) -> List[Tuple[List[str], str]]:
        """Generate agent scale test commands"""
        commands = []
        if test_specs.agent_scale_tests:
            for agent_scale_test in test_specs.agent_scale_tests:
                if agent_scale_test.common_test_spec.asic_test_options:
                    for (
                        asic_type,
                        asic_options,
                    ) in agent_scale_test.common_test_spec.asic_test_options.items():
                        test_commands = (
                            self.test_runner.build_agent_scale_test_commands(
                                agent_scale_test, asic_type, asic_options
                            )
                        )
                        commands.extend(test_commands)
        return commands

    def _generate_warmboot_test_commands(
        self, test_specs
    ) -> List[Tuple[List[str], str]]:
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

    def _generate_link_test_commands(self, test_specs) -> List[Tuple[List[str], str]]:
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

    def _generate_config_test_commands(self, test_specs) -> List[Tuple[List[str], str]]:
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

    def _generate_benchmark_test_commands(
        self, test_specs
    ) -> List[Tuple[List[str], str]]:
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

    def _generate_test_summary_report(self) -> None:
        """Generate test summary report from log files based on test runner mode"""
        log_dir = self.log_dir
        if not log_dir:
            print("Warning: Log directory not initialized, skipping summary report")
            return

        # Create the appropriate summary report generator
        test_runner_mode = self.config.test_runner_mode
        if test_runner_mode is None:
            print("Warning: Test runner mode not set, skipping summary report")
            return

        report_generator = create_summary_report_generator(test_runner_mode.value)

        # Generate the summary report (file names are determined by the generator)
        report_generator.generate_summary_report(log_dir)
