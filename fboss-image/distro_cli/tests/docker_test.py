"""Test Docker infrastructure."""

import unittest

from distro_cli.lib.constants import FBOSS_BUILDER_IMAGE
from distro_cli.lib.docker import container
from distro_cli.tests.test_helpers import ensure_test_docker_image


class TestDockerInfrastructure(unittest.TestCase):
    """Test Docker infrastructure basics."""

    @classmethod
    def setUpClass(cls):
        """Ensure fboss_builder image exists before running tests."""
        ensure_test_docker_image()

    def test_run_container(self):
        """Test running a simple container command."""
        exit_code = container.run_container(
            image=FBOSS_BUILDER_IMAGE,
            command=["echo", "hello from run_container"],
            ephemeral=True,
        )
        self.assertEqual(exit_code, 0)

    def test_exec_in_container(self):
        """Test executing a command in a running container."""
        exit_code = container.run_container(
            image=FBOSS_BUILDER_IMAGE,
            command=["sleep", "inf"],
            ephemeral=False,
            detach=True,
            name="test_exec_container",
        )
        self.assertEqual(exit_code, 0)

        exec_exit_code, stdout, stderr = container.exec_in_container(
            name="test_exec_container",
            command=["echo", "hello from exec_in_container"],
        )
        self.assertEqual(exec_exit_code, 0)
        self.assertEqual(stdout.strip(), "hello from exec_in_container")

        # Clean up the container
        container.stop_and_remove_container(name="test_exec_container")

        # Check if container is stopped and removed
        is_running = container.container_is_running("test_exec_container")
        self.assertFalse(is_running)

        # Try to exec in the removed container - should fail with non-zero exit code
        exec_exit_code, stdout, stderr = container.exec_in_container(
            name="test_exec_container",
            command=["echo", "should not work"],
        )
        self.assertNotEqual(exec_exit_code, 0)

    def test_container_is_running(self):
        """Test checking if a container is running."""
        # Check non-existent container
        is_running = container.container_is_running("non_existent_container")
        self.assertFalse(is_running)

        # Start a container
        exit_code = container.run_container(
            image=FBOSS_BUILDER_IMAGE,
            command=["sleep", "1"],
            ephemeral=False,
            name="test_running_container",
        )
        self.assertEqual(exit_code, 0)

        # Check if container is running
        is_running = container.container_is_running("test_running_container")
        self.assertTrue(is_running)

        # Stop and remove the container
        container.stop_and_remove_container(name="test_running_container")

        # Check if container is stopped and removed
        is_running = container.container_is_running("test_running_container")
        self.assertFalse(is_running)

    def test_stop_and_remove_container(self):
        """Test stopping and removing a container."""
        # Start a container
        exit_code = container.run_container(
            image=FBOSS_BUILDER_IMAGE,
            command=["sleep", "1"],
            ephemeral=False,
            name="test_stop_and_remove_container",
        )
        self.assertEqual(exit_code, 0)

        # Check if container is running
        is_running = container.container_is_running("test_stop_and_remove_container")
        self.assertTrue(is_running)

        # Stop and remove the container
        exit_code = container.stop_and_remove_container(
            name="test_stop_and_remove_container"
        )
        self.assertEqual(exit_code, 0)

        # Check if container is stopped and removed
        is_running = container.container_is_running("test_stop_and_remove_container")
        self.assertFalse(is_running)


if __name__ == "__main__":
    unittest.main()
