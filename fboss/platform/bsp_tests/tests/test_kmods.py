# pyre-unsafe
import pytest

from fboss.platform.bsp_tests.utils.kmod_utils import (
    fbsp_remove,
    get_loaded_kmods,
    load_kmods,
    unload_kmods,
)


def test_load_kmods(platform_config) -> None:
    load_kmods(platform_config.kmods)


def test_unload_kmods(platform_config) -> None:
    unload_kmods(platform_config.kmods)


@pytest.mark.stress
def test_kmod_load_unload_stress(platform_config) -> None:
    for _ in range(100):
        load_kmods(platform_config.kmods)
        unload_kmods(platform_config.kmods)


def test_fbsp_remove(platform_config) -> None:
    load_kmods(platform_config.kmods)
    try:
        fbsp_remove(platform_config.vendor)
    except Exception as e:
        pytest.fail(f"Unexpected exception {e}")
    loaded_kmods = get_loaded_kmods(platform_config.kmods)
    assert len(loaded_kmods) == 0
