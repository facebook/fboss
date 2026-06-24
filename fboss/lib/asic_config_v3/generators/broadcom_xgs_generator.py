# pyre-strict

import json
import os
from typing import Any

import yaml
from fboss.lib.asic_config_v3.base_generator import BaseAsicConfigGenerator, MODULE_DIR
from fboss.lib.platform_mapping_v2.gen import read_all_vendor_data
from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingParser


class BroadcomXgsGenerator(BaseAsicConfigGenerator):
    """Data-driven ASIC config generator for the Broadcom XGS family.

    Produces multi-document YAML output with typed tables.
    """

    ASIC_FAMILY: str = "xgs"

    def __init__(
        self,
        platform_name: str,
        variant: str,
        platform_config: dict[str, Any],
    ) -> None:
        super().__init__(platform_name, variant, platform_config)

        self.values: dict[str, Any] = {}
        self.asic_config: dict[str, Any] = {}

        self._load_vendor_configs()
        self._init_tables()

        self.parser = PlatformMappingParser(read_all_vendor_data(), self.platform_name)

        self.num_ports_per_core: int = self.platform_config.get("num_ports_per_core", 2)
        self.mmu_size: int = self.asic_config.get("mmu_size", 9416)

        # Compute lanes_per_port from ASIC port_architecture and platform num_ports_per_core
        port_arch = self.asic_config.get("port_architecture", {})
        num_lanes_per_core = port_arch.get("num_lanes_per_core", 8)
        self.lanes_per_port: int = num_lanes_per_core // self.num_ports_per_core

    @property
    def output_extension(self) -> str:
        return ".yml"

    def _load_vendor_configs(self) -> None:
        """Load the family-wide OCP, SDK, SAI common blocks and the per-ASIC config."""
        vendor = self.asic_vendor
        asic = self.asic_name

        ocp_sai_common_path = os.path.join(MODULE_DIR, "common", "ocp_sai_common.json")
        if os.path.exists(ocp_sai_common_path):
            with open(ocp_sai_common_path) as f:
                self.ocp_sai_common = json.load(f)
        else:
            self.ocp_sai_common = {"global": {}}

        vendor_sdk_common_path = os.path.join(
            MODULE_DIR, "vendors", vendor, self.ASIC_FAMILY, "sdk_common.json"
        )
        with open(vendor_sdk_common_path) as f:
            self.vendor_sdk_common = json.load(f)

        vendor_sai_common_path = os.path.join(
            MODULE_DIR, "vendors", vendor, self.ASIC_FAMILY, "sai_common.json"
        )
        with open(vendor_sai_common_path) as f:
            self.vendor_sai_common = json.load(f)

        asic_config_path = os.path.join(
            MODULE_DIR, "vendors", vendor, self.ASIC_FAMILY, "asics", f"{asic}.json"
        )
        with open(asic_config_path) as f:
            self.asic_config = json.load(f)

        self.asic_name = self.asic_config.get("asic", asic)

    def _init_tables(self) -> None:
        """Initialize empty dictionaries for each table."""
        table_names = self.asic_config.get("table_names", [])
        for table_name in table_names:
            self.values[table_name] = {}

    def _read_preamble(self, file_name: str) -> str:
        """Read preamble file contents."""
        preamble_path = os.path.join(MODULE_DIR, file_name)
        with open(preamble_path, encoding="utf-8") as f:
            return f.read()

    def get_lane_map_str(self, lane_map: list[int]) -> str:
        """Convert a lane map list to its little-endian hex string form.

        For example, [3, 2, 1, 0, 7, 6, 5, 4] becomes "0x45670123".
        """
        lane_map_str = "".join(str(lane) for lane in reversed(lane_map))
        return f"0x{lane_map_str}"

    def get_polarity_map_str(self, pn_map: list[int]) -> str:
        """Convert a polarity map (per-lane binary flags) to a hex byte string.

        The polarity bits are read little-endian and packed into a single byte
        rendered as "0x" plus two uppercase hex digits.
        """
        pn_map_str = "".join(str(pn) for pn in reversed(pn_map))
        return f"0x{int(pn_map_str, 2):02X}"

    def _apply_pass_through_settings(self) -> None:
        """Copy each pass-through source block to its target output table.

        A variant may declare a top-level dict at the same source key to
        replace the ASIC default for that pass-through entirely. This lets a
        platform emit a subset or different values than the chip-wide defaults.
        """
        for pt in self.asic_config.get("pass_through_settings", []):
            source_key = pt["source"]
            target_table = pt["target_table"]
            data = self.variant_config.get(
                source_key, self.asic_config.get(source_key, {})
            )
            self.values[target_table].update(data)

    def _compute_skip_from_sai_common(self) -> list[str]:
        """Collect skip_from_sai_common fields from all active conditional settings."""
        skip_fields: list[str] = []
        for cs in self.asic_config.get("conditional_settings", []):
            condition = cs.get("condition", {})
            source = condition.get("source", "asic_config_params")
            if source == "asic_config_params":
                param_value = self.asic_config_params.get(condition["param"])
            elif source == "features":
                param_value = self.variant_config.get("features", {}).get(
                    condition["param"]
                )
            else:
                continue
            if param_value == condition.get("equals"):
                skip_fields.extend(cs.get("skip_from_sai_common", []))
        return skip_fields

    def _apply_conditional_settings(self) -> None:
        """Evaluate and apply conditional settings.

        ASIC-level entries from ``asic_config.conditional_settings`` are
        evaluated first, then platform-level entries from
        ``variant_config.conditional_settings``. Within each list, matching
        entries are applied in array order. Platform entries are applied after
        ASIC entries so platform settings can override chip-level ones.
        """
        asic_entries = self.asic_config.get("conditional_settings", [])
        platform_entries = self.variant_config.get("conditional_settings", [])
        for cs in list(asic_entries) + list(platform_entries):
            condition = cs.get("condition", {})
            source = condition.get("source", "asic_config_params")
            if source == "asic_config_params":
                param_value = self.asic_config_params.get(condition["param"])
            elif source == "features":
                param_value = self.variant_config.get("features", {}).get(
                    condition["param"]
                )
            else:
                continue
            if param_value != condition.get("equals"):
                continue

            for table_name, entries in cs.get("apply", {}).items():
                self.values.setdefault(table_name, {}).update(entries)

            apply_from = cs.get("apply_from")
            if apply_from:
                source_key = apply_from["source"]
                target_table = apply_from["target_table"]
                self.values[target_table].update(self.asic_config.get(source_key, {}))

    def _apply_layered_global_settings(self) -> None:
        """Apply layered settings to the ``global`` table. Higher-numbered steps override lower."""
        # OCP SAI common (vendor-agnostic SAI standard defaults)
        self.values["global"].update(self.ocp_sai_common.get("global", {}))

        # ASIC global defaults (chip-specific base settings)
        self.values["global"].update(self.asic_config.get("global_defaults", {}))

        # Vendor SDK common (shared BCM settings)
        self.values["global"].update(self.vendor_sdk_common.get("global", {}))

        # Vendor SAI common (with skip_from_sai_common filtering)
        skip_fields = self._compute_skip_from_sai_common()
        for key, value in self.vendor_sai_common.get("global", {}).items():
            if key in skip_fields:
                continue
            self.values["global"][key] = value

        # ASIC SAI overrides (chip-specific SAI overrides)
        self.values["global"].update(self.asic_config.get("sai_overrides", {}))

        # Platform-level SAI overrides from the variant config, layered
        # on top of the chip-level SAI overrides so platform tuning wins.
        self.values["global"].update(
            self.variant_config.get("platform_sai_overrides", {})
        )

    def _get_core_range(self) -> list[int]:
        """Return the list of core indices to include in the port mapping.

        Honors the ``core_range`` override (a comma-separated list of inclusive
        ranges such as ``"0-15,48-63"``) when present, otherwise falls back to
        the ASIC's full core count.
        """
        port_mapping_overrides = self.variant_config.get("port_mapping_overrides", {})
        core_range_str = port_mapping_overrides.get("core_range", None)

        if core_range_str:
            cores = []
            for part in core_range_str.split(","):
                if "-" in part:
                    start, end = part.split("-")
                    cores.extend(range(int(start), int(end) + 1))
                else:
                    cores.append(int(part))
            return cores
        port_arch = self.asic_config.get("port_architecture", {})
        num_cores = port_arch.get("num_cores", 64)
        return list(range(num_cores))

    def _get_logical_port_to_physical_port_mapping(self) -> list[list[int]]:
        """Compute the logical-to-physical port mapping for this variant.

        The mapping formula is driven by the ASIC's ``port_architecture`` and
        may be tuned per variant via ``port_mapping_overrides``:

          * ``num_logical_ports_per_datapath``: override the ASIC-level value.
          * ``num_lp_ports_on_even_core``: override the ASIC-level default.
          * ``lp_start_step_offset``: offset added to the logical-ports-per-
            datapath value when computing the per-datapath logical-port start.
            Defaults to 1.
          * ``lp_offset_simple``: when true, compute the per-port logical-port
            offset as ``i * lanes_per_port`` on all cores rather than using
            the default even / odd conditional.
        """
        port_arch = self.asic_config.get("port_architecture", {})
        port_mapping_overrides = self.variant_config.get("port_mapping_overrides", {})

        lp_per_dp = port_mapping_overrides.get(
            "num_logical_ports_per_datapath",
            port_arch.get("num_logical_ports_per_datapath", 10),
        )
        pp_per_dp = port_arch.get("num_physical_ports_per_datapath", 16)
        lp_on_even_core = port_mapping_overrides.get(
            "num_lp_ports_on_even_core",
            port_arch.get("num_lp_ports_on_even_core_default", 8),
        )
        lanes_per_core = port_arch.get("num_lanes_per_core", 8)
        lp_start_step_offset = port_mapping_overrides.get("lp_start_step_offset", 1)
        lp_offset_simple = port_mapping_overrides.get("lp_offset_simple", False)

        logical_to_physical_port_mapping = []
        cores = self._get_core_range()

        for core_num in cores:
            core_offset = 1 if core_num <= 1 else 0
            lp_start_offset = 0 if core_num % 2 == 0 else lp_on_even_core
            lp_start = (
                (core_num // 2) * (lp_per_dp + lp_start_step_offset)
                + lp_start_offset
                + core_offset
            )
            pp_start_offset = 0 if core_num % 2 == 0 else lanes_per_core
            pp_start = ((core_num // 2) * pp_per_dp) + 1 + pp_start_offset

            for i in range(self.num_ports_per_core):
                if lp_offset_simple:
                    lp_offset = i * self.lanes_per_port
                else:
                    lp_offset = i * (self.lanes_per_port if core_num % 2 == 0 else 1)
                pp_offset = i * self.lanes_per_port
                lp_id = lp_start + lp_offset
                pp_id = pp_start + pp_offset
                logical_to_physical_port_mapping.append([lp_id, pp_id])

        return logical_to_physical_port_mapping

    def _generate_logical_port_to_physical_port_mapping(
        self, mgmt_port: bool = False
    ) -> None:
        """Generate PC_PORT_PHYS_MAP entries."""
        lp_mapping = self._get_logical_port_to_physical_port_mapping()

        # Reserve the CPU port row for logical 0 to physical 0 mapping.
        lp_mapping.append([0, 0])

        for lp_map in lp_mapping:
            pm_key = (f"PORT_ID: {lp_map[0]}",)
            pm_value = (f"PC_PHYS_PORT_ID: {lp_map[1]}",)
            self.values["PC_PORT_PHYS_MAP"][pm_key] = pm_value

        if mgmt_port:
            mgmt_defaults = self.asic_config.get("mgmt_port_defaults", {})
            mgmt_overrides = self.variant_config.get("mgmt_port_overrides", {})
            logical_id = mgmt_overrides.get(
                "logical_id", mgmt_defaults.get("logical_id", 76)
            )
            physical_id = mgmt_overrides.get(
                "physical_id", mgmt_defaults.get("physical_id", 513)
            )
            pm_key = (f"PORT_ID: {logical_id}",)
            pm_value = (f"PC_PHYS_PORT_ID: {physical_id}",)
            self.values["PC_PORT_PHYS_MAP"][pm_key] = pm_value

    def _generate_lane_map(self) -> None:
        """Generate lane map entries in PC_PM_CORE."""
        static_mapping = self.parser.get_static_mapping().get_static_mapping()
        lane_maps = static_mapping.phy_lane_map

        for core_id, lane_info in lane_maps.items():
            tx_lane_info, rx_lane_info = lane_info.tx_lane_info, lane_info.rx_lane_info
            pm_key = (f"PC_PM_ID: {core_id + 1}", "CORE_INDEX: 0")
            pm_value = (
                f"RX_LANE_MAP: {self.get_lane_map_str(list(rx_lane_info))}",
                "RX_LANE_MAP_AUTO: 0",
                f"TX_LANE_MAP: {self.get_lane_map_str(list(tx_lane_info))}",
                "TX_LANE_MAP_AUTO: 0",
            )
            pm_value = self.values["PC_PM_CORE"].get(pm_key, ()) + pm_value
            self.values["PC_PM_CORE"][pm_key] = pm_value

    def _generate_polarity_map(self) -> None:
        """Generate polarity map entries in PC_PM_CORE."""
        static_mapping = self.parser.get_static_mapping().get_static_mapping()
        pn_swap_maps = static_mapping.polarity_swap_map

        for core_id, swap_map in pn_swap_maps.items():
            tx_swap_map, rx_swap_map = swap_map.tx_lane_info, swap_map.rx_lane_info
            pm_key = (f"PC_PM_ID: {core_id + 1}", "CORE_INDEX: 0")
            pm_value = (
                f"RX_POLARITY_FLIP: {self.get_polarity_map_str(list(rx_swap_map))}",
                "RX_POLARITY_FLIP_AUTO: 0",
                f"TX_POLARITY_FLIP: {self.get_polarity_map_str(list(tx_swap_map))}",
                "TX_POLARITY_FLIP_AUTO: 0",
            )
            pm_value = self.values["PC_PM_CORE"].get(pm_key, ()) + pm_value
            self.values["PC_PM_CORE"][pm_key] = pm_value

    def _generate_port_config(self, mgmt_port: bool = False) -> None:
        """Generate PC_PORT and PORT entries."""
        port_config = self.variant_config.get("port_config", {})
        cpu_port_config = self.variant_config.get("cpu_port", {})
        mgmt_port_config = self.variant_config.get("mgmt_port", {})
        mgmt_defaults = self.asic_config.get("mgmt_port_defaults", {})
        mgmt_overrides = self.variant_config.get("mgmt_port_overrides", {})

        default_speed = port_config.get("default_speed", 400000)
        speed_to_fec = port_config.get("speed_to_fec", {})

        cpu_speed = cpu_port_config.get("speed", 10000)
        cpu_num_lanes = cpu_port_config.get("num_lanes", 1)
        self.values["PC_PORT"][(f"PORT_ID: {0}",)] = (
            "ENABLE: 1",
            f"SPEED: {cpu_speed}",
            f"NUM_LANES: {cpu_num_lanes}",
        )

        fec = speed_to_fec.get(str(default_speed), "PC_FEC_RS544_2XN")
        fp_enable = port_config.get("enable", 0)
        port_ranges = [
            f"[{lp_map[0]}, {lp_map[0]}]"
            for lp_map in self._get_logical_port_to_physical_port_mapping()
        ]
        port_ranges_str = ", ".join(port_ranges)
        pc_key = (f"PORT_ID: [{port_ranges_str}]",)
        pc_value = (
            f"ENABLE: {fp_enable}",
            f"SPEED: {default_speed}",
            f"NUM_LANES: {self.lanes_per_port}",
            f"FEC_MODE: {fec}",
            f"MAX_FRAME_SIZE: {self.mmu_size}",
        )
        self.values["PC_PORT"][pc_key] = pc_value

        if mgmt_port and mgmt_port_config.get("enabled", False):
            logical_id = mgmt_overrides.get(
                "logical_id", mgmt_defaults.get("logical_id", 76)
            )
            mgmt_speed = mgmt_overrides.get("speed", mgmt_defaults.get("speed", 100000))
            mgmt_num_lanes = mgmt_overrides.get(
                "num_lanes", mgmt_defaults.get("num_lanes", 4)
            )
            mgmt_fec = mgmt_overrides.get(
                "fec", mgmt_defaults.get("fec", "PC_FEC_RS528")
            )
            mgmt_enable = mgmt_port_config.get("enable", 0)

            mgmt_port_key = (f"PORT_ID: {logical_id}",)
            mgmt_port_value = (
                f"ENABLE: {mgmt_enable}",
                f"SPEED: {mgmt_speed}",
                f"NUM_LANES: {mgmt_num_lanes}",
                f"FEC_MODE: {mgmt_fec}",
                f"MAX_FRAME_SIZE: {self.mmu_size}",
            )
            self.values["PC_PORT"][mgmt_port_key] = mgmt_port_value

            # Optionally include the mgmt port in the PORT (MTU) range alongside FP.
            if mgmt_port_config.get("in_port_block", False):
                port_ranges.append(f"[{logical_id}, {logical_id}]")
                port_ranges_str = ", ".join(port_ranges)
                pc_key = (f"PORT_ID: [{port_ranges_str}]",)

        self.values["PORT"][pc_key] = (
            f"MTU: {self.mmu_size}",
            "MTU_CHECK: 1",
        )

    def _generate_device_config_overrides(self) -> None:
        """Apply device config overrides from variant config."""
        self.values["DEVICE_CONFIG"].update(
            self.variant_config.get("device_config_overrides", {})
        )

    def generate(self) -> str:
        """Build the full ASIC config and return it as a YAML string.

        The XGS layering defines a nine-step priority order. Later steps
        override earlier steps when a key appears in more than one source:

          1. Pass-through settings (named data blocks routed to output tables).
          2. OCP SAI common.
          3. ASIC global defaults.
          4. Vendor SDK common.
          5. Vendor SAI common (with skip_from_sai_common filtering).
          6. ASIC SAI overrides, then platform-level SAI overrides.
          7. Conditional settings (feature toggles).
          8. Device config overrides (per-variant DEVICE_CONFIG entries).
          9. Lane and polarity maps from platform_mapping_v2 (into PC_PM_CORE).

        The numbering describes *priority*, not execution order. The layered
        global settings (steps 2-6) are applied before pass-through settings
        so keys that both contribute to the ``global`` dict appear in a
        deterministic positional order. Pass-through-only tables (PORT_CONFIG,
        FP_CONFIG, TM_THD_CONFIG, CTR_EFLEX_CONFIG) are disjoint from the
        global layering and unaffected by this choice.

        Port mapping and port-config generation target disjoint tables
        (PC_PORT_PHYS_MAP, PC_PORT, PORT) and stand outside the nine-step
        priority order.
        """
        self._apply_layered_global_settings()
        self._apply_pass_through_settings()
        self._apply_conditional_settings()
        self._generate_device_config_overrides()
        self._generate_logical_port_to_physical_port_mapping(mgmt_port=True)
        self._generate_port_config(mgmt_port=True)
        self._generate_lane_map()
        self._generate_polarity_map()

        return self._generate_yaml_string()

    def _generate_yaml_string(self) -> str:
        """Render the populated tables as a multi-document YAML string."""
        table_names = self.asic_config.get("table_names", [])
        unit = 0
        tables = []

        for table_name in table_names:
            if table_name.startswith(
                (
                    "PC_",
                    "PORT_CONFIG",
                    "TM_THD_CONFIG",
                    "PORT",
                    "DEVICE_CONFIG",
                    "FP_CONFIG",
                    "CTR_EFLEX_CONFIG",
                    "DLB_ECMP_CONFIG",
                )
            ):
                device = "device"
            else:
                device = "bcm_device"

            if table_value := self.values.get(table_name):
                tables.append({device: {unit: {table_name: table_value}}})

        yaml_str = yaml.dump_all(
            tables, sort_keys=False, explicit_start=True, explicit_end=True
        )

        # Strip tuple tags and bullet-style indentation introduced by yaml.dump
        # so the output matches the legacy format produced by asic_config_v2.
        yaml_str = yaml_str.replace(" !!python/tuple", "")
        yaml_str = yaml_str.replace("- ", "  ")
        yaml_str = yaml_str.replace("'", "")

        preamble_str = ""
        if self.asic_config.get("preamble_support", False):
            preamble_file = self.variant_config.get("preamble_file")
            if preamble_file:
                preamble_str = self._read_preamble(preamble_file)

        return preamble_str + yaml_str
