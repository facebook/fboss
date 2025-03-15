# pyre-strict
# pytest fixtures are way too complicated to typecheck properly with pyre
import os

from pathlib import Path

import pytest
from fboss.platform.bsp_tests.cdev_types import FpgaSpec, I2CAdapter
from fboss.platform.bsp_tests.config import ConfigLib, RuntimeConfig, TestConfig
from fboss.platform.bsp_tests.test_runner import PLATFORMS
from fboss.platform.bsp_tests.utils.kmod_utils import (
    fbsp_remove,
    load_kmods,
    read_kmods,
)

platform_to_vendor = {
    "janga800bic": "fboss",
    "meru800bfa": "arista",
    "meru800bia": "arista",
    "montblanc": "fboss",
    "morgan800cc": "cisco",
    "tahan800bc": "fboss",
}


def pytest_configure(config) -> None:  # pyre-ignore config
    config.addinivalue_line("markers", "stress: mark test as a stress test")


def pytest_addoption(parser) -> None:  # pyre-ignore parser
    parser.addoption("--config_file", action="store", default=None)
    parser.addoption("--install_dir", action="store", default="")
    parser.addoption("--config_subdir", action="store", default="configs")
    parser.addoption("--pm_config_dir", action="store", default="")
    parser.addoption("--platform", action="store", default=None)
    parser.addoption("--tests-subdir", type=str, default="tests")


@pytest.fixture(scope="session")
def platform_config(request) -> RuntimeConfig:  # pyre-ignore request
    platform = request.config.getoption("platform")

    config_file = request.config.getoption("config_file")
    install_dir = request.config.getoption("install_dir")
    config_subdir = request.config.getoption("config_subdir")
    pm_config_dir = request.config.getoption("pm_config_dir")

    configLib = ConfigLib(pm_config_dir)

    config_file_path = ""
    test_conf = None
    try:
        if config_file:
            config_file_path = config_file
        else:
            if platform not in PLATFORMS:
                raise Exception(
                    f"Unknown platform '{platform}'. Available platforms are {PLATFORMS}"
                )
            config_file_path = os.path.join(
                install_dir, config_subdir, f"{platform}.json"
            )
        with open(Path(config_file_path)) as f:
            json_string = f.read()
        test_conf = TestConfig.from_json(json_string)  # pyre-ignore
    except Exception as e:
        print(f"Failed to read config file {config_file_path}: {e}")
        test_conf = TestConfig(
            platform=platform, testDevices=[], vendor=platform_to_vendor[platform]
        )

    kmods = read_kmods(test_conf.vendor)

    pm_conf = configLib.get_platform_manager_config(test_conf.platform)
    runtime_conf = configLib.build_runtime_config(test_conf, pm_conf, kmods)

    return runtime_conf


@pytest.fixture(scope="session")
def platform_fpgas(platform_config: RuntimeConfig) -> list[FpgaSpec]:
    return [fpga.device for fpga in platform_config.fpgas]


@pytest.fixture(scope="session")
def fpga_with_adapters(
    platform_fpgas: list[FpgaSpec],
) -> list[tuple[FpgaSpec, I2CAdapter]]:
    return [(fpga, adapter) for fpga in platform_fpgas for adapter in fpga.i2cAdapters]


@pytest.fixture(autouse=True)
def load_modules(platform_config: RuntimeConfig) -> None:
    load_kmods(platform_config.kmods)


@pytest.fixture(scope="module", autouse=True)
def clean_modules(platform_config: RuntimeConfig) -> None:
    # Ensures a clean running environment (e.g. no lingering devices)
    # for each test
    try:
        fbsp_remove(platform_config.vendor)
    except Exception:
        # Ignore errors if fbsp-remove.sh is not present, will fail
        # the specific test for this
        pass
