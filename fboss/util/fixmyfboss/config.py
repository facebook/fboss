# pyre-strict
import importlib.resources
import json
from typing import Optional

from fboss.platform.platform_manager.platform_manager_config.thrift_types import (
    PlatformConfig,
)
from fboss.util.fixmyfboss.platform_name_lib import get_platform_name
from thrift.py3 import Protocol, serializer


def load_pm_config() -> Optional[PlatformConfig]:
    """
    Load platform manager configs for all platforms
    Returns a dictionary mapping platform names to their PlatformConfig objects
    """

    parent_dir = "fboss_config_files/configs"
    platform_name = get_platform_name()
    if not platform_name:
        raise Exception("Failed to get platform name")

    platform_name_lower = platform_name.lower()
    filename = "platform_manager.json"
    config_path = f"{parent_dir}/{platform_name_lower}/{filename}"
    resource = importlib.resources.files(__package__).joinpath(config_path)

    pm_config = None
    try:
        with resource.open("r") as config_file:
            config_content = config_file.read()
            pm_config = serializer.deserialize(
                PlatformConfig,
                json.dumps(json.loads(config_content)).encode("utf-8"),
                protocol=Protocol.JSON,
            )
    except Exception as e:
        print(f"Warning: Failed to load config for {resource.name}: {e}")

    return pm_config
