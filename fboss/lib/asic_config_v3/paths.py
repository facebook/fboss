# pyre-strict

import os
from dataclasses import dataclass


@dataclass(frozen=True)
class AsicConfigPaths:
    fboss_root: str

    @classmethod
    def from_root(cls, fboss_root: str) -> "AsicConfigPaths":
        return cls(os.path.abspath(fboss_root))

    @property
    def module_dir(self) -> str:
        return os.path.join(self.fboss_root, "lib", "asic_config_v3")

    @property
    def schemas_dir(self) -> str:
        return os.path.join(self.module_dir, "schemas")

    @property
    def configs_dir(self) -> str:
        return os.path.join(self.fboss_root, "configs")

    @property
    def asic_vendors_dir(self) -> str:
        return os.path.join(self.configs_dir, "asic_vendors")

    @property
    def platforms_dir(self) -> str:
        return os.path.join(self.configs_dir, "platforms")

    @property
    def platform_mapping_dir(self) -> str:
        return os.path.join(self.fboss_root, "lib", "platform_mapping_v2", "platforms")
