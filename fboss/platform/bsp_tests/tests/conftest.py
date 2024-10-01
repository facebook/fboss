import os
from pathlib import Path
from typing import List, Tuple

import pytest
from fboss.platform.bsp_tests.test_runner import Config, FpgaSpec, PLATFORMS
from fboss.platform.bsp_tests.utils.cdev_types import I2CAdapter
from fboss.platform.bsp_tests.utils.kmod_utils import fbsp_remove, load_kmods


def pytest_configure(config):
    config.addinivalue_line("markers", "stress: mark test as a stress test")


def pytest_addoption(parser):
    parser.addoption("--config_file", action="store", default=None)
    parser.addoption("--install_dir", action="store", default="")
    parser.addoption("--config_subdir", action="store", default="configs")
    parser.addoption("--platform", action="store", default=None)
    parser.addoption("--tests-subdir", type=str, default="tests")


@pytest.fixture(scope="session")
def platform_config(request):
    platform = request.config.getoption("platform")

    config_file = request.config.getoption("config_file")
    install_dir = request.config.getoption("install_dir")
    config_subdir = request.config.getoption("config_subdir")

    config_file_path = ""
    if config_file:
        config_file_path = config_file
    else:
        if platform not in PLATFORMS:
            raise Exception(
                f"Unknown platform '{platform}'. Available platforms are {PLATFORMS}"
            )
        config_file_path = os.path.join(install_dir, config_subdir, f"{platform}.json")

    with open(Path(config_file_path)) as f:
        json_string = f.read()
    return Config.from_json(json_string)


@pytest.fixture(scope="session")
def platform_fpgas(platform_config):
    return platform_config.fpgas


@pytest.fixture(scope="session")
def fpga_with_adapters(platform_fpgas) -> list[tuple[FpgaSpec, I2CAdapter]]:
    return [(fpga, adapter) for fpga in platform_fpgas for adapter in fpga.i2cAdapters]


@pytest.fixture(autouse=True)
def load_modules(platform_config):
    load_kmods(platform_config.kmods)


@pytest.fixture(scope="module", autouse=True)
def clean_modules():
    # Ensures a clean running environment (e.g. no lingering devices)
    # for each test
    try:
        fbsp_remove()
    except Exception:
        # Ignore errors if fbsp-remove.sh is not present, will fail
        # the specific test for this
        pass
