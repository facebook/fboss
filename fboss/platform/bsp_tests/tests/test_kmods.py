import pytest

from fboss.platform.bsp_tests.test_runner import TestBase
from fboss.platform.bsp_tests.utils.kmod_utils import (
    fbsp_remove,
    get_loaded_kmods,
    load_kmods,
    unload_kmods,
)


class TestKmods(TestBase):
    def test_load_kmods(self, platform_config) -> None:
        load_kmods(platform_config.kmods)

    def test_unload_kmods(self, platform_config) -> None:
        unload_kmods(platform_config.kmods)

    @pytest.mark.stress
    def test_kmod_load_unload_stress(self, platform_config) -> None:
        for _ in range(100):
            load_kmods(platform_config.kmods)
            unload_kmods(platform_config.kmods)

    def test_fbsp_remove(self, platform_config) -> None:
        load_kmods(platform_config.kmods)
        assert fbsp_remove()
        loaded_kmods = get_loaded_kmods(platform_config.kmods)
        assert len(loaded_kmods) == 0
