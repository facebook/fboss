from typing import List

import pytest

from fboss.platform.bsp_tests.test_runner import TestBase
from fboss.platform.bsp_tests.utils.cmd_utils import check_cmd
from fboss.platform.bsp_tests.utils.kmod_utils import fbsp_remove, get_loaded_kmods


class TestKmods(TestBase):
    kmods: List[str] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.kmods = cls.config.kmods

    def test_load_kmods(self) -> None:
        self.load_kmods()

    def test_unload_kmods(self) -> None:
        self.unload_kmods()

    @pytest.mark.stress
    def test_kmod_load_unload_stress(self) -> None:
        for _ in range(100):
            self.load_kmods()
            self.unload_kmods()

    def test_fbsp_remove(self) -> None:
        self.load_kmods()
        assert fbsp_remove()
        loaded_kmods = get_loaded_kmods(self.kmods)
        assert len(loaded_kmods) == 0

    def load_kmods(self) -> None:
        for kmod in self.kmods:
            check_cmd(["modprobe", kmod])

    def unload_kmods(self) -> None:
        for kmod in reversed(self.kmods):
            check_cmd(["modprobe", "-r", kmod])
