#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import annotations

import enum
import logging
import re
import typing as t
from datetime import datetime, timedelta

import MySQLdb
from libfb.py.employee import get_current_unix_user_fbid
from neteng.fboss.phy.phy.types import PhyInfo, LinkSnapshot
from neteng.fboss.transceiver.types import TransceiverInfo
from nettools.nowa.building_blocks.all.ngt.link_check.common import ssh_util
from rfe.client_py3 import get_client as get_rfe_client
from rfe.RockfortExpress.types import QueryCommon
from thrift.py3 import deserialize, Protocol

T = t.TypeVar("T")

NGT_SERVICE_FBID = 89002005347827

SQL = """
SELECT
    `msg`
FROM `network_event_log`
WHERE
    ((`device_name`) IN ('{}'))
    AND ((`originator`) IN ('wedge_agent', 'qsfp_service'))
    AND (CONTAINS(`msg`, ARRAY('linkSnapshot')))
    AND (CONTAINS(`msg`, ARRAY('<port:{}>')))
    AND `timestamp` BETWEEN {} and {}
"""

SNAPSHOT_FORMAT = r"{}LINK_SNAPSHOT_EVENT(LINK_SNAPSHOT): Collected snapshot for ports  <port:{}>  <linkSnapshot:{}>"
SNAPSHOT_REGEX = (
    SNAPSHOT_FORMAT.replace("(", r"\(")
    .replace(")", r"\)")
    .replace(r"<port:{}> ", r"(<port:{}> )+")
    .format(r".*", r"eth\d+/\d+/\d+", r"(.*)")
)

DEFAULT_TIME_RANGE = timedelta(minutes=1)
FETCH_SNAPSHOT_LOG_COMMAND = "zgrep 'LINK_SNAPSHOT_EVENT' {} | grep {} | grep -v sshd"

ARCHIVE_PATH = "/var/facebook/logs/fboss/archive/"

AGENT_ARCHIVE_FMT = ARCHIVE_PATH + "wedge_agent.log-{}.gz"
AGENT_ARCHIVE_PATTERN = AGENT_ARCHIVE_FMT.format("*")
AGENT_CURRENT_LOG = "/var/facebook/logs/fboss/wedge_agent.log"

QSFP_ARCHIVE_FMT = ARCHIVE_PATH + "qsfp_service.log-{}.gz"
QSFP_ARCHIVE_PATTERN = QSFP_ARCHIVE_FMT.format("*")
QSFP_CURRENT_LOG = "/var/facebook/logs/fboss/qsfp_service.log"

FILENAME_REGEX = (
    ARCHIVE_PATH
    + r"(?:wedge_agent|qsfp_service).log-(\d\d\d\d)(\d\d)(\d\d)(\d\d)(\d\d).gz"
)

DEFAULT_LOGGER = logging.getLogger()
DEFAULT_LOGGER.setLevel("INFO")


class Backend(enum.Enum):
    SCUBA = enum.auto()
    SSH = enum.auto()


class SnapshotCollection:
    def __init__(
        self,
        iphy_snaps: t.Mapping[int, PhyInfo],
        xphy_snaps: t.Mapping[int, PhyInfo],
        tcvr_snaps: t.Mapping[int, TransceiverInfo],
    ):
        self._iphy_snaps: t.Mapping[int, PhyInfo] = iphy_snaps
        self._xphy_snaps: t.Mapping[int, PhyInfo] = xphy_snaps
        self._tcvr_snaps: t.Mapping[int, TransceiverInfo] = tcvr_snaps

    # unpacks (iphy, xphy, transceiver) snapshots
    def unpack(
        self,
    ) -> t.Tuple[
        t.Mapping[int, PhyInfo],
        t.Mapping[int, PhyInfo],
        t.Mapping[int, TransceiverInfo],
    ]:
        return self._iphy_snaps, self._xphy_snaps, self._tcvr_snaps

    def empty(self) -> bool:
        return not (
            len(self._iphy_snaps) or len(self._xphy_snaps) or len(self._tcvr_snaps)
        )


def escape_sql(sql: str) -> str:
    return MySQLdb.escape_string(sql.encode()).decode()


# convert Datetime object to nanosecond timestamp
def datetime_to_ns(dt: datetime) -> int:
    return int(dt.timestamp() * 1e9)


def filter_timeseries(
    mapping: t.Mapping[int, T], time_start: datetime, time_end: datetime
) -> t.Mapping[int, T]:
    return {
        ts: v
        for ts, v in mapping.items()
        if time_start.timestamp() <= ts <= time_end.timestamp()
    }


class SnapshotClient:
    def __init__(self, user: int, logger: logging.Logger = DEFAULT_LOGGER):
        self._user = user
        self._logger: logging.Logger = logger

    async def get_logfiles_in_timeframe(
        self, hostname: str, time_start: datetime, time_end: datetime
    ) -> t.List[str]:
        # returns the mapping of timestamp -> logfile
        def parse_timestamps(filenames: t.List[str]) -> t.List[t.Tuple[float, str]]:
            timestamps: t.List[t.Tuple[float, str]] = []
            for filename in filenames:
                match = re.fullmatch(FILENAME_REGEX, filename)
                if match is None:
                    raise Exception(
                        "File has invalid timestamp format\nFile name was: {}".format(
                            filename
                        )
                    )
                else:
                    dt = datetime(
                        year=int(match.group(1)),
                        month=int(match.group(2)),
                        day=int(match.group(3)),
                        hour=int(match.group(4)),
                        minute=int(match.group(5)),
                    )
                    timestamps.append((dt.timestamp(), filename))
            return timestamps

        # Filter which logfiles could possibly contain the given timeframe.
        def filter_by_timestamp(
            log_timestamps: t.List[t.Tuple[float, str]], current_log: str
        ) -> t.List[str]:
            possible_logfiles: t.List[str] = []
            timestamp: float = 0
            for timestamp, filename in log_timestamps:
                # get all log files ending between the ranges, along with the first one after time_end
                if (
                    time_start.timestamp() <= timestamp <= time_end.timestamp()
                    or timestamp >= time_end.timestamp()
                ):
                    possible_logfiles.append(filename)
                if timestamp >= time_end.timestamp():
                    break

            if time_end.timestamp() >= log_timestamps[-1][0]:
                possible_logfiles.append(current_log)

            return possible_logfiles

        logfiles = (
            (
                await ssh_util.run_ssh_cmd(
                    hostname,
                    f"ls {AGENT_ARCHIVE_PATTERN} {QSFP_ARCHIVE_PATTERN}",
                    self._logger,
                )
            )
            .strip()
            .split("\n")
        )

        log_timestamps = parse_timestamps(logfiles)

        return filter_by_timestamp(
            [(ts, log) for ts, log in log_timestamps if "wedge_agent" in log],
            AGENT_CURRENT_LOG,
        ) + filter_by_timestamp(
            [(ts, log) for ts, log in log_timestamps if "qsfp_service" in log],
            QSFP_CURRENT_LOG,
        )

    # fetches snapshots within the past 3 hours
    async def fetch_recent_snapshots(
        self,
        hostname: str,
        port_name: str,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_around_time(
            hostname, port_name, datetime.now(), timedelta(hours=3), backend
        )

    # Fetch the snapshots posted between (timestamp - time_delta) and (timestamp + time_delta)
    # Note that for scuba this filters based on the time that the snapshot was posted,
    #   and not the time that the snapshot was collected
    async def fetch_snapshots_around_time(
        self,
        hostname: str,
        port_name: str,
        timestamp: datetime,
        time_delta: timedelta = DEFAULT_TIME_RANGE,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_in_timespan(
            hostname, port_name, timestamp - time_delta, timestamp + time_delta, backend
        )

    async def fetch_snapshots_in_timespan(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        self._logger.info(
            f"Fetching snapshots for host {hostname}:{port_name} between {time_start.isoformat()} and {time_end.isoformat()} via {str(backend)})"
        )
        if backend == Backend.SSH:
            return await self.fetch_snapshots_in_timespan_via_ssh(
                hostname, port_name, time_start, time_end
            )
        elif backend == Backend.SCUBA:
            return await self.fetch_snapshots_in_timespan_via_scuba(
                hostname, port_name, time_start, time_end
            )
        else:
            raise Exception(
                f"Invalid backend passed to fetch_snapshots_in_timespan: {str(backend)}"
            )

    async def fetch_snapshots_in_timespan_via_ssh(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
    ) -> SnapshotCollection:
        possible_logfiles: t.List[str] = await self.get_logfiles_in_timeframe(
            hostname, time_start, time_end
        )
        cmd = FETCH_SNAPSHOT_LOG_COMMAND.format(" ".join(possible_logfiles), port_name)

        output = (
            await ssh_util.run_ssh_cmd(
                hostname,
                cmd,
                self._logger,
                len(possible_logfiles) * ssh_util.BYTES_STDOUT_LIMIT,
            )
        ).strip()
        collection = await self.process_snapshot_lines(output.split("\n"))

        iphy, xphy, tcvr = collection.unpack()
        iphy = filter_timeseries(iphy, time_start, time_end)
        xphy = filter_timeseries(xphy, time_start, time_end)
        tcvr = filter_timeseries(tcvr, time_start, time_end)

        return SnapshotCollection(iphy, xphy, tcvr)

    async def fetch_snapshots_in_timespan_via_scuba(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
    ) -> SnapshotCollection:
        qc = QueryCommon(
            source="snapshot_manager",
            user_id=self._user,
            instance="network_event_log",
        )
        sql = SQL.format(
            escape_sql(hostname),
            escape_sql(port_name),
            datetime_to_ns(time_start),
            datetime_to_ns(time_end),
        )
        async with get_rfe_client() as client:
            scuba_entries = await client.querySQL(qc, sql)

        lines = [row[0] for row in scuba_entries.value]
        return await self.process_snapshot_lines(lines)

    async def process_snapshot_lines(self, lines: t.List[str]) -> SnapshotCollection:
        iphy_snapshots: t.Dict[int, PhyInfo] = {}
        xphy_snapshots: t.Dict[int, PhyInfo] = {}
        tcvr_snapshots: t.Dict[int, TransceiverInfo] = {}
        for line in lines:
            match = re.fullmatch(SNAPSHOT_REGEX, line)
            if match is None:
                raise Exception(
                    "link snapshot has invalid format\nSnapshot was: {}".format(line)
                )
            else:
                snapshot_str = match.group(2)

            # TODO(ccpowers): remove this line once we fix qsfp service to
            # not read compliance code on CMIS modules (which don't support it)

            snapshot_str = snapshot_str.replace(
                '"extendedSpecificationComplianceCode":32,', ""
            )

            snapshot = deserialize(
                LinkSnapshot, snapshot_str.encode(), protocol=Protocol.JSON
            )
            if snapshot.type is LinkSnapshot.Type.phyInfo:
                ts = snapshot.phyInfo.timeCollected
                if snapshot.phyInfo.system is None:
                    iphy_snapshots[ts] = snapshot.phyInfo
                else:
                    xphy_snapshots[ts] = snapshot.phyInfo
            elif snapshot.type is LinkSnapshot.Type.transceiverInfo:
                ts = snapshot.transceiverInfo.timeCollected
                if ts is None:
                    raise Exception("No timestamp found for transceiver snapshots")
                tcvr_snapshots[ts] = snapshot.transceiverInfo
            else:
                raise Exception("Invalid type for link snapshot: ", snapshot.type)
        return SnapshotCollection(iphy_snapshots, xphy_snapshots, tcvr_snapshots)


async def get_client(logger: logging.Logger = DEFAULT_LOGGER) -> SnapshotClient:
    return SnapshotClient(
        user=get_current_unix_user_fbid() or NGT_SERVICE_FBID, logger=logger
    )
