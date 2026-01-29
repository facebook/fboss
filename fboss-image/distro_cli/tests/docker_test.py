"""Test Docker infrastructure."""

import unittest

from distro_cli.lib.constants import FBOSS_BUILDER_IMAGE
from distro_cli.lib.docker.container import run_container
from distro_cli.tests.test_helpers import ensure_test_docker_image


class TestDockerInfrastructure(unittest.TestCase):
    """Test Docker infrastructure basics."""

    @classmethod
    def setUpClass(cls):
        """Ensure fboss_builder image exists before running tests."""
        ensure_test_docker_image()

    def test_run_simple_container(self):
        """Test running a simple container command."""
        exit_code = run_container(
            image=FBOSS_BUILDER_IMAGE,
            command=["echo", "hello from container"],
            ephemeral=True,
        )
        self.assertEqual(exit_code, 0)


if __name__ == "__main__":
    unittest.main()
