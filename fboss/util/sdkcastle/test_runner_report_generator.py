#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Test runner log parser and summary report generator for sdkcastle
#

# pyre-unsafe

import re
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class BaseTestRunnerSummaryReportGenerator(ABC):
    """Base class for test runner summary report generators"""

    @abstractmethod
    def generate_summary_report(self, log_dir: Path) -> None:
        """
        Generate test summary report from log files.

        Args:
            log_dir: Directory containing the test log files and commands file
        """
        pass

    @abstractmethod
    def _get_commands_filename(self) -> str:
        """
        Get the commands file name for this test runner.

        Returns:
            Commands file name (e.g., 'netcastle_commands.txt')
        """
        pass

    @abstractmethod
    def _get_output_filename(self) -> str:
        """
        Get the output summary report file name for this test runner.

        Returns:
            Output file name (e.g., 'netcastle_test_summary_report.txt')
        """
        pass

    @abstractmethod
    def _parse_log_file(self, log_file_path: Path) -> Dict[str, Optional[str]]:
        """
        Parse a test log file to extract exit code and test summary.

        Args:
            log_file_path: Path to the test log file

        Returns:
            Dictionary containing parsed information (exit_code, test_summary, etc.)
        """
        pass

    def _parse_commands_file(self, commands_file: Path) -> List[Tuple[str, str]]:
        """
        Parse commands file to extract command and log file pairs.

        Args:
            commands_file: Path to commands file

        Returns:
            List of (command, log_filename) tuples
        """
        command_log_pairs = []

        try:
            with open(commands_file, "r") as f:
                lines = f.readlines()

            current_command = None
            current_log = None

            for line in lines:
                line = line.strip()

                # Skip empty lines and "Command N:" headers
                if not line or line.startswith("Command "):
                    continue

                # Check if this is a log file line
                if line.startswith("Log file:"):
                    current_log = line.replace("Log file:", "").strip()
                    # If we have both command and log, save the pair
                    if current_command and current_log:
                        command_log_pairs.append((current_command, current_log))
                        current_command = None
                        current_log = None
                else:
                    # This is a command line
                    current_command = line

        except Exception as e:
            print(f"Error parsing commands file {commands_file}: {e}")

        return command_log_pairs


class NetcastleSummaryReportGenerator(BaseTestRunnerSummaryReportGenerator):
    """Summary report generator for netcastle test runner (meta-internal mode)"""

    def _get_commands_filename(self) -> str:
        """Get the commands file name for netcastle test runner."""
        return "netcastle_commands.txt"

    def _get_output_filename(self) -> str:
        """Get the output summary report file name for netcastle test runner."""
        return "netcastle_test_summary_report.txt"

    def _get_failed_tests_filename(self) -> str:
        """Get the failed tests report file name for netcastle test runner."""
        return "netcastle_failed_tests.txt"

    def generate_summary_report(self, log_dir: Path) -> None:
        """
        Generate netcastle test summary report from log files.

        Args:
            log_dir: Directory containing the netcastle log files and commands file
        """
        commands_file = log_dir / self._get_commands_filename()
        output_file = log_dir / self._get_output_filename()

        # Print the  message
        print("\n=== Generating Netcastle Test Summary Report ===")

        if not commands_file.exists():
            print(f"Commands file not found: {commands_file}")
            return

        try:
            # Parse commands file to get command-to-logfile mapping
            command_log_pairs = self._parse_commands_file(commands_file)

            # Generate summary report
            with open(output_file, "w") as f:
                f.write("=" * 80 + "\n")
                f.write("Netcastle Test Summary Report\n")
                f.write("=" * 80 + "\n\n")

                for i, (command, log_filename) in enumerate(command_log_pairs, 1):
                    f.write(f"Command {i}:\n")
                    f.write(f"{command}\n")
                    f.write(f"Log file: {log_filename}\n")

                    # Parse the log file
                    log_file_path = log_dir / log_filename
                    parse_result = self._parse_log_file(log_file_path)

                    # Write exit code
                    exit_code = parse_result.get("exit_code")
                    if exit_code:
                        f.write(f"Netcastle exit code: Exit with {exit_code}\n")
                    else:
                        f.write("Netcastle exit code: NOT FOUND\n")

                    # Write test summary
                    test_summary = parse_result.get("test_summary")
                    if test_summary:
                        f.write("\nTest Results Summary:\n")
                        f.write(test_summary + "\n")
                    else:
                        f.write("\nTest Results Summary: NOT FOUND\n")

                    f.write("\n" + "-" * 80 + "\n\n")

            print(f"Generated summary report: {output_file}")

            # Generate failed tests report
            self._generate_failed_tests_report(log_dir, command_log_pairs)

        except Exception as e:
            print(f"Error generating summary report: {e}")

    def _generate_failed_tests_report(
        self, log_dir: Path, command_log_pairs: List[Tuple[str, str]]
    ) -> None:
        """
        Generate netcastle failed tests report from log files.

        This report lists all FAILED and TIMEOUT test cases for each command,
        making it easy to identify problematic tests across all test runs.

        Args:
            log_dir: Directory containing the netcastle log files
            command_log_pairs: List of (command, log_filename) tuples
        """
        failed_tests_file = log_dir / self._get_failed_tests_filename()

        print("\n=== Generating Netcastle Failed Tests Report ===")

        try:
            with open(failed_tests_file, "w") as f:
                f.write("=" * 80 + "\n")
                f.write("Netcastle Failed and Timeout Tests Report\n")
                f.write("=" * 80 + "\n\n")

                for i, (command, log_filename) in enumerate(command_log_pairs, 1):
                    f.write(f"Command {i}:\n")
                    f.write(f"{command}\n")
                    f.write(f"Log file: {log_filename}\n\n")

                    # Parse the log file to extract failed and timeout tests
                    log_file_path = log_dir / log_filename
                    failed_tests = self._extract_failed_tests_from_log(log_file_path)

                    # Write failed test cases list
                    f.write("Failed test cases list:\n")
                    if failed_tests:
                        for test_name in failed_tests:
                            f.write(f"  - {test_name}\n")
                    else:
                        f.write("  No FAILED/TIMEOUT test cases\n")

                    f.write("\n" + "-" * 80 + "\n\n")

            print(f"Generated failed tests report: {failed_tests_file}")

        except Exception as e:
            print(f"Error generating failed tests report: {e}")

    def _extract_failed_tests_from_log(self, log_file_path: Path) -> List[str]:
        """
        Extract failed and timeout test cases from a netcastle log file.

        Args:
            log_file_path: Path to the netcastle log file

        Returns:
            List of failed and timeout test case names
        """
        if not log_file_path.exists():
            return []

        try:
            with open(log_file_path, "r") as f:
                lines = f.readlines()

            return self._extract_failed_and_timeout_tests(lines)

        except Exception as e:
            print(f"Error extracting failed tests from {log_file_path}: {e}")
            return []

    def _parse_log_file(self, log_file_path: Path) -> Dict[str, Optional[str]]:
        """
        Parse a netcastle log file to extract exit code and test summary.

        Args:
            log_file_path: Path to the netcastle log file

        Returns:
            Dictionary containing:
            - 'exit_code': The exit code string (e.g., 'SUCCESS', 'TEST_CASE_FAILURE')
            - 'test_summary': The test results summary (multi-line string)
        """
        result: Dict[str, Optional[str]] = {"exit_code": None, "test_summary": None}

        if not log_file_path.exists():
            return result

        try:
            with open(log_file_path, "r") as f:
                lines = f.readlines()

            # Find "Exit with" line
            exit_code = self._extract_exit_code(lines)
            if exit_code:
                result["exit_code"] = exit_code

            # Find test summary starting with "Ran ... tests"
            test_summary = self._extract_test_summary(lines)
            if test_summary:
                result["test_summary"] = test_summary

        except Exception as e:
            print(f"Error parsing log file {log_file_path}: {e}")

        return result

    @staticmethod
    def _extract_exit_code(lines: List[str]) -> Optional[str]:
        """
        Extract exit code from lines containing "Exit with".

        Args:
            lines: List of lines from the log file

        Returns:
            Exit code string (e.g., 'SUCCESS', 'TEST_CASE_FAILURE') or None
        """
        exit_pattern = re.compile(r"Exit with\s+(\S+)")

        for line in lines:
            match = exit_pattern.search(line)
            if match:
                return match.group(1)

        return None

    @staticmethod
    def _extract_test_summary(lines: List[str]) -> Optional[str]:
        """
        Extract test summary starting from "Ran ... tests" line.

        The summary includes lines like:
        - Ran 559 tests in 3h 17m 13s
        - [     ERROR ] 0
        - [    FAILED ] 10
        - [   TIMEOUT ] 0
        - [    PASSED ] 549
        - [   SKIPPED ] 0
        - [   OMITTED ] 0
        - [   RETRIED ] 0

        Args:
            lines: List of lines from the log file

        Returns:
            Multi-line test summary string or None
        """
        summary_lines = []
        in_summary = False
        ran_pattern = re.compile(r"^Ran\s+\d+\s+tests")
        # Pattern to match lines like "[     ERROR ] 0" or "[    PASSED ] 549"
        result_pattern = re.compile(
            r"^\[\s*(ERROR|FAILED|TIMEOUT|PASSED|SKIPPED|OMITTED|RETRIED)\s*\]"
        )

        for line in lines:
            # Check if this line starts the summary
            if ran_pattern.match(line.strip()):
                in_summary = True
                summary_lines.append(line.rstrip())
                continue

            # If we're in the summary section, collect lines
            if in_summary:
                stripped = line.strip()
                # Continue collecting lines that look like test result stats
                # or are part of the summary (non-empty lines)
                if stripped:
                    # Check if line matches result patterns like "[     ERROR ] 0"
                    if result_pattern.match(stripped):
                        summary_lines.append(line.rstrip())
                    elif summary_lines:
                        # Stop if we hit a line that doesn't match expected patterns
                        # after we've started collecting
                        break
                else:
                    # Empty line might indicate end of summary
                    if summary_lines:
                        break

        return "\n".join(summary_lines) if summary_lines else None

    @staticmethod
    def _extract_failed_and_timeout_tests(lines: List[str]) -> List[str]:
        """
        Extract FAILED and TIMEOUT test cases from the "Test Results for Config:" section.

        In netcastle logs, the test results section starts with "Test Results for Config:"
        and lists FAILED tests first (starting with "[  FAILED] "), then TIMEOUT tests
        (starting with "[ TIMEOUT]"), followed by other test results.

        Args:
            lines: List of lines from the log file

        Returns:
            List of failed and timeout test case names
        """
        failed_and_timeout_tests = []
        in_test_results_section = False
        test_results_pattern = re.compile(r"Test Results for Config:")
        failed_pattern = re.compile(r"^\[\s*FAILED\s*\]\s+(.+)$")
        timeout_pattern = re.compile(r"^\[\s*TIMEOUT\s*\]\s+(.+)$")
        # Pattern to detect when we've moved past FAILED/TIMEOUT to other results
        other_result_pattern = re.compile(r"^\[\s*(PASSED|ERROR|SKIPPED|OMITTED)\s*\]")
        # Pattern to detect the summary section (which comes after test results)
        ran_pattern = re.compile(r"^Ran\s+\d+\s+tests")

        for line in lines:
            stripped = line.strip()

            # Check if we're entering the test results section
            if test_results_pattern.search(stripped):
                in_test_results_section = True
                continue

            # If we're in the test results section
            if in_test_results_section:
                # Check if we've reached the summary section (end of test results)
                if ran_pattern.match(stripped):
                    break

                # Check for FAILED test
                failed_match = failed_pattern.match(stripped)
                if failed_match:
                    test_name = failed_match.group(1).strip()
                    failed_and_timeout_tests.append(test_name)
                    continue

                # Check for TIMEOUT test
                timeout_match = timeout_pattern.match(stripped)
                if timeout_match:
                    test_name = timeout_match.group(1).strip()
                    failed_and_timeout_tests.append(test_name)
                    continue

                # If we hit other result types (PASSED, ERROR, etc.), we're done
                # with FAILED/TIMEOUT section
                if other_result_pattern.match(stripped):
                    break

        return failed_and_timeout_tests


class OSSSummaryReportGenerator(BaseTestRunnerSummaryReportGenerator):
    """Summary report generator for OSS test runner (oss mode)"""

    def _get_commands_filename(self) -> str:
        """Get the commands file name for OSS test runner."""
        return "oss_commands.txt"

    def _get_output_filename(self) -> str:
        """Get the output summary report file name for OSS test runner."""
        return "oss_test_summary_report.txt"

    def generate_summary_report(self, log_dir: Path) -> None:
        """
        Generate OSS test summary report from log files.

        Args:
            log_dir: Directory containing the OSS test log files and commands file

        Note:
            This is a stub implementation. Full implementation will be added
            when OSS test runner log format details are available.
        """
        commands_file = log_dir / self._get_commands_filename()
        output_file = log_dir / self._get_output_filename()

        # Print the  message
        print("\n=== Generating OSS Test Summary Report ===")

        if not commands_file.exists():
            print(f"Commands file not found: {commands_file}")
            return

        try:
            # Parse commands file to get command-to-logfile mapping
            command_log_pairs = self._parse_commands_file(commands_file)

            # Generate summary report
            with open(output_file, "w") as f:
                f.write("=" * 80 + "\n")
                f.write("OSS Test Summary Report\n")
                f.write("=" * 80 + "\n\n")

                for i, (command, log_filename) in enumerate(command_log_pairs, 1):
                    f.write(f"Command {i}:\n")
                    f.write(f"{command}\n")
                    f.write(f"Log file: {log_filename}\n")

                    # Parse the log file
                    log_file_path = log_dir / log_filename
                    parse_result = self._parse_log_file(log_file_path)

                    # Write exit code (stub)
                    exit_code = parse_result.get("exit_code")
                    if exit_code:
                        f.write(f"Exit code: {exit_code}\n")
                    else:
                        f.write("Exit code: NOT FOUND\n")

                    # Write test summary (stub)
                    test_summary = parse_result.get("test_summary")
                    if test_summary:
                        f.write("\nTest Results Summary:\n")
                        f.write(test_summary + "\n")
                    else:
                        f.write("\nTest Results Summary: NOT FOUND\n")

                    f.write("\n" + "-" * 80 + "\n\n")

            print(f"Generated summary report: {output_file}")

        except Exception as e:
            print(f"Error generating summary report: {e}")

    def _parse_log_file(self, log_file_path: Path) -> Dict[str, Optional[str]]:
        """
        Parse an OSS test log file to extract exit code and test summary.

        Args:
            log_file_path: Path to the OSS test log file

        Returns:
            Dictionary containing:
            - 'exit_code': The exit code or status
            - 'test_summary': The test results summary

        Note:
            This is a stub implementation. Full implementation will be added
            when OSS test runner log format details are available.
        """
        result: Dict[str, Optional[str]] = {"exit_code": None, "test_summary": None}

        if not log_file_path.exists():
            return result

        # TODO: Implement OSS log parsing logic when format details are available
        # For now, return empty results
        return result


def create_summary_report_generator(
    test_runner_mode: str,
) -> BaseTestRunnerSummaryReportGenerator:
    """
    Function to create the appropriate summary report generator based on test runner mode.

    Args:
        test_runner_mode: The test runner mode ('meta-internal' or 'oss')

    Returns:
        Instance of the appropriate summary report generator

    Raises:
        ValueError: If test_runner_mode is not recognized
    """
    if test_runner_mode == "meta-internal":
        return NetcastleSummaryReportGenerator()
    elif test_runner_mode == "oss":
        return OSSSummaryReportGenerator()
    else:
        raise ValueError(f"Unknown test runner mode: {test_runner_mode}")
