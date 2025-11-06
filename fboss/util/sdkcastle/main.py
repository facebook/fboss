#
# Copyright 2004-present Facebook. All Rights Reserved.
#
# Main invocation for sdkcastle testing tool
#

# pyre-unsafe

import argparse
import logging
import sys
from pathlib import Path

from .config_parser import ConfigParser
from .test_executor import TestExecutor


def setup_logging(log_level: str) -> logging.Logger:
    """Setup logging configuration"""
    # Convert string to logging level
    numeric_level = getattr(logging, log_level.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError(f"Invalid log level: {log_level}")

    # Configure logging
    logging.basicConfig(
        level=numeric_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    # Create and return logger
    logger = logging.getLogger("sdkcastle")
    return logger


def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description="SDKCastle - ASIC vendor SDK testing tool"
    )
    parser.add_argument(
        "--config",
        "-c",
        required=True,
        help="Path to configuration JSON file",
    )
    parser.add_argument(
        "--log-level",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        default="INFO",
        help="Set the logging level (default: INFO)",
    )

    args = parser.parse_args()

    # Set log level based on the log-level
    log_level = args.log_level
    logger = setup_logging(log_level)

    try:
        # Parse configuration
        config_path = Path(args.config)
        if not config_path.exists():
            logger.error(f"Configuration file not found: {config_path}")
            sys.exit(1)

        config_parser = ConfigParser()
        config = config_parser.parse_from_file(str(config_path))

        logger.info(f"Loaded configuration from: {config_path}")
        logger.debug(f"SDK Version: {config.test_sdk_version}")
        logger.debug(f"Run Mode: {config.run_mode}")
        logger.debug(f"Build Mode: {config.build_mode}")
        logger.debug(f"Test Runner Mode: {config.test_runner_mode}")

        # Execute tests
        executor = TestExecutor(config)
        result = executor.execute()

        logger.info(f"Execution completed with status: {result['status']}")

        # Exit with appropriate code
        if result["status"] == "success":
            sys.exit(0)
        else:
            sys.exit(1)

    except Exception as e:
        logger.error(f"Execution failed: {e}")
        logger.debug("Full traceback:", exc_info=True)
        sys.exit(1)


if __name__ == "__main__":
    main()
