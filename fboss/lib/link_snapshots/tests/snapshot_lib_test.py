import typing as t
from datetime import datetime, timedelta

import fboss.lib.link_snapshots.snapshot_lib as snapshot_lib
from fboss.lib.link_snapshots.snapshot_lib import (
    SNAPSHOT_FORMAT,
    Backend,
    AGENT_ARCHIVE_FMT,
    QSFP_ARCHIVE_FMT,
)
from later.unittest.mock import AsyncContextManager, AsyncMock, patch
from libfb.py.asyncio.await_utils import await_sync
from libfb.py.asyncio.unittest import TestCase
from neteng.fboss.phy.phy.types import (
    LinkSnapshot,
    PhyInfo,
    DataPlanePhyChip,
    DataPlanePhyChipType,
    PhySideInfo,
    Side,
)
from neteng.fboss.transceiver.types import TransceiverInfo
from remcmd.thrift.remcmd import types as remcmd_types
from RockfortExpress import RockfortExpress as rfe
from thrift.py3 import serialize, Protocol

# Unfortunately, it doesn't seem like we'd really be able to test out the SQL
# filtering logic here, as that would need to be processed by an actual MySQL DB
# but we can at least unit test some of the other method logic

NUM_SNAPSHOTS = 10
HOSTNAME = "test_host"
TCVR_ID = 1
PORT_ID = 2
PORT_NAME = "eth2/1/1"

# redefined here instead of importing from snapshot_lib to catch log name typos
AGENT_CURRENT_LOG = "/var/facebook/logs/fboss/wedge_agent.log"
QSFP_CURRENT_LOG = "/var/facebook/logs/fboss/qsfp_service.log"


def build_iphy_snapshot(timestamp: int, port_name: str) -> LinkSnapshot:
    chip = DataPlanePhyChip(name="PHY", type=DataPlanePhyChipType.IPHY, physicalID=0)
    line = PhySideInfo(side=Side.LINE)
    phy_info = PhyInfo(phyChip=chip, name=port_name, line=line, timeCollected=timestamp)
    return LinkSnapshot(phyInfo=phy_info)


def build_xphy_snapshot(timestamp: int, port_name: str) -> LinkSnapshot:
    chip = DataPlanePhyChip(name="PHY", type=DataPlanePhyChipType.IPHY, physicalID=0)
    line = PhySideInfo(side=Side.LINE)
    system = PhySideInfo(side=Side.SYSTEM)
    phy_info = PhyInfo(
        phyChip=chip, name=port_name, line=line, timeCollected=timestamp, system=system
    )
    return LinkSnapshot(phyInfo=phy_info)


def build_transceiver_snapshot(timestamp: int, tcvr_id: int) -> LinkSnapshot:
    tcvr_info = TransceiverInfo(port=tcvr_id, timeCollected=timestamp)
    return LinkSnapshot(transceiverInfo=tcvr_info)


class SnapshotLibTest(TestCase):
    def setUp(self) -> None:
        super().setUp()
        # snapshot client
        self.client = await_sync(snapshot_lib.get_client())
        self.snapshot_lines = self.mock_snapshots()
        self.unprocessed_snapshots = "\n".join(self.snapshot_lines)

        self.mock_grep_result = remcmd_types.JobResult(
            status=remcmd_types.JobStatus.FINISHED,
            results=[
                remcmd_types.TaskResult(
                    stderr="task results test stderr",
                    stdout="\n".join(self.snapshot_lines),
                    exit_code=0,
                    status=remcmd_types.TaskStatus.FINISHED,
                )
            ],
            tasks_failed=0,
            tasks_succeeded=0,
            tasks_remaining=0,
            tasks_total=1,
            tasks_unknown=0,
        )

        # mock the logfile listing
        agent_timestamps: t.List[datetime] = [
            datetime(year=2021, month=1, day=1, hour=0),
            datetime(year=2021, month=1, day=2, hour=0),
            datetime(year=2021, month=1, day=3, hour=0),
            datetime(year=2021, month=1, day=4, hour=0),
        ]
        qsfp_timestamps: t.List[datetime] = [
            datetime(year=2021, month=1, day=1, hour=12),
            datetime(year=2021, month=1, day=2, hour=12),
            datetime(year=2021, month=1, day=3, hour=12),
            datetime(year=2021, month=1, day=4, hour=12),
        ]
        files = [
            AGENT_ARCHIVE_FMT.format(dt.strftime("%Y%m%d%H%M"))
            for dt in agent_timestamps
        ] + [
            QSFP_ARCHIVE_FMT.format(dt.strftime("%Y%m%d%H%M")) for dt in qsfp_timestamps
        ]

        self.mock_ls_result = remcmd_types.JobResult(
            status=remcmd_types.JobStatus.FINISHED,
            results=[
                remcmd_types.TaskResult(
                    stderr="task results test stderr",
                    stdout="\n".join(files),
                    exit_code=0,
                    status=remcmd_types.TaskStatus.FINISHED,
                )
            ],
            tasks_failed=0,
            tasks_succeeded=0,
            tasks_remaining=0,
            tasks_total=1,
            tasks_unknown=0,
        )

        # mock Scuba
        self.rfe_client = AsyncMock()
        self.rfe_client.querySQL.return_value = rfe.SQLQueryResult(
            value=[[snapshot_line] for snapshot_line in self.snapshot_lines]
        )

    # Snapshot_lib makes two ssh calls, one to list the logfiles, one to
    # fetch the snapshots from the logs. We need to have different mock output
    # depending on the command
    def mocked_ssh(
        self, client_id: str, shell_cmd: t.Optional[str], **kwargs
    ) -> remcmd_types.JobResult:
        if shell_cmd is None:
            raise Exception("mocked_ssh expects shell_cmd, was None")

        if "ls " in shell_cmd:
            return self.mock_ls_result
        elif "grep" in shell_cmd:
            return self.mock_grep_result
        else:
            raise Exception("Unhandled command in mocked ssh: ", shell_cmd)

    def mock_snapshots(self) -> t.List[str]:
        mock_snapshots = []
        base_timestamp = int(datetime.now().timestamp())
        for i in range(NUM_SNAPSHOTS):
            iphy_snapshot = build_iphy_snapshot(base_timestamp + i, PORT_NAME)
            xphy_snapshot = build_xphy_snapshot(base_timestamp + i, PORT_NAME)
            tcvr_snapshot = build_transceiver_snapshot(base_timestamp + i, TCVR_ID)

            mock_snapshots.append(iphy_snapshot)
            mock_snapshots.append(xphy_snapshot)
            mock_snapshots.append(tcvr_snapshot)

        snapshot_lines = [
            SNAPSHOT_FORMAT.format(
                "",
                "eth2/1/1",
                serialize(snapshot, protocol=Protocol.JSON).decode(),
            )
            for snapshot in mock_snapshots
        ]

        return snapshot_lines

    async def test_snapshot_types(self) -> None:
        snapshots_to_test = [
            self.unprocessed_snapshots.split("\n"),
            self.unprocessed_snapshots.replace(
                r"<port:eth2/1/1>",
                r"<port:eth2/1/1> <port:eth2/1/2> <port:eth2/1/3> <port:eth2/1/4>",
            ).split("\n"),
        ]
        for test_snapshot in snapshots_to_test:
            collection = await self.client.process_snapshot_lines(test_snapshot)
            iphy, xphy, tcvr = collection.unpack()
            iphy_list = list(iphy.items())
            time, snapshot = iphy_list[0]
            self.assertTrue(
                type(snapshot) is PhyInfo or type(snapshot) is TransceiverInfo
            )
            self.assertTrue(type(time) is int)

    @patch(
        "nettools.nowa.building_blocks.all.ngt.link_check.common.ssh_util.simple_run"
    )
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_recent_snapshots(
        self, mock_get_rfe_client, mock_ssh_collection
    ) -> None:
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=self.rfe_client, instance=True
        )

        # Test scuba

        # We're using fetch_recent_snapshots for simplicity. We dont go to an
        # actual SQL engine so we never check the timestamps in these tests
        snapshot_collection = await self.client.fetch_recent_snapshots(
            HOSTNAME, PORT_NAME, backend=Backend.SCUBA
        )
        iphy_snapshots, xphy_snapshots, tcvr_snapshots = snapshot_collection.unpack()

        self.assertEqual(len(iphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(xphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(tcvr_snapshots), NUM_SNAPSHOTS)

        # test ssh
        mock_ssh_collection.side_effect = self.mocked_ssh

        snapshot_collection = await self.client.fetch_recent_snapshots(
            HOSTNAME, PORT_NAME, backend=Backend.SSH
        )
        iphy_snapshots, xphy_snapshots, tcvr_snapshots = snapshot_collection.unpack()

        self.assertEqual(len(iphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(xphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(tcvr_snapshots), NUM_SNAPSHOTS)

    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_around_time_scuba(self, mock_get_rfe_client) -> None:
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=self.rfe_client, instance=True
        )
        # test default param
        res = await self.client.fetch_snapshots_around_time(
            HOSTNAME, PORT_NAME, datetime.now(), backend=Backend.SCUBA
        )
        self.assertEqual(type(res), snapshot_lib.SnapshotCollection)

        # test non-default param
        res = await self.client.fetch_snapshots_around_time(
            HOSTNAME,
            PORT_NAME,
            datetime.now(),
            timedelta(seconds=1),
            backend=Backend.SCUBA,
        )
        self.assertEqual(type(res), snapshot_lib.SnapshotCollection)

    @patch(
        "nettools.nowa.building_blocks.all.ngt.link_check.common.ssh_util.simple_run"
    )
    async def test_fetch_snapshots_around_time_onbox(self, mock_ssh_collection) -> None:
        mock_ssh_collection.side_effect = self.mocked_ssh

        res = await self.client.fetch_snapshots_around_time(
            HOSTNAME, PORT_NAME, datetime.now(), backend=Backend.SSH
        )
        self.assertEqual(type(res), snapshot_lib.SnapshotCollection)

        # test non-default param
        res = await self.client.fetch_snapshots_around_time(
            HOSTNAME,
            PORT_NAME,
            datetime.now(),
            timedelta(seconds=1),
            backend=Backend.SSH,
        )
        self.assertEqual(type(res), snapshot_lib.SnapshotCollection)

    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_in_timespan_scuba(self, mock_get_rfe_client) -> None:
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=self.rfe_client, instance=True
        )
        res = await self.client.fetch_snapshots_around_time(
            HOSTNAME,
            PORT_NAME,
            datetime.now(),
            timedelta(seconds=0),
            backend=Backend.SCUBA,
        )
        self.assertEqual(type(res), snapshot_lib.SnapshotCollection)

    # Test that we only grep the files that could possibly contain the timespan
    # we're querying.
    @patch(
        "nettools.nowa.building_blocks.all.ngt.link_check.common.ssh_util.simple_run"
    )
    async def test_log_timespan_filtering(self, mock_ssh_cmd) -> None:
        mock_ssh_cmd.side_effect = self.mocked_ssh

        # before the earliest timestamp, should only return first agent and qsfp archive logs
        res = await self.client.get_logfiles_in_timeframe(
            HOSTNAME,
            datetime(year=2020, month=12, day=20),
            datetime(year=2020, month=12, day=25),
        )
        self.assertEqual(
            res,
            [
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101010000.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101011200.gz",
            ],
        )

        # after last archive timestamp, should only return current logs
        res = await self.client.get_logfiles_in_timeframe(
            HOSTNAME,
            datetime(year=2021, month=1, day=4, hour=15),
            datetime(year=2021, month=1, day=4, hour=16),
        )
        self.assertEqual(res, [AGENT_CURRENT_LOG, QSFP_CURRENT_LOG])

        # before 1/2 but after 1/1 agent file, and before 1/1 qsfp file
        res = await self.client.get_logfiles_in_timeframe(
            HOSTNAME,
            datetime(year=2021, month=1, day=1, hour=5),
            datetime(year=2021, month=1, day=1, hour=6),
        )
        self.assertEqual(
            res,
            [
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101020000.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101011200.gz",
            ],
        )

        # should return all files up until agent 1/2 and qsfp 1/1
        res = await self.client.get_logfiles_in_timeframe(
            HOSTNAME,
            datetime(year=2020, month=12, day=31, hour=0),
            datetime(year=2021, month=1, day=1, hour=6),
        )
        self.assertEqual(
            res,
            [
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101010000.gz",
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101020000.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101011200.gz",
            ],
        )

        # should contain all of the logs
        res = await self.client.get_logfiles_in_timeframe(
            HOSTNAME,
            datetime(year=2020, month=12, day=31, hour=0),
            datetime(year=2021, month=1, day=6, hour=0),
        )
        self.assertEqual(
            res,
            [
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101010000.gz",
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101020000.gz",
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101030000.gz",
                "/var/facebook/logs/fboss/archive/wedge_agent.log-202101040000.gz",
                AGENT_CURRENT_LOG,
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101011200.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101021200.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101031200.gz",
                "/var/facebook/logs/fboss/archive/qsfp_service.log-202101041200.gz",
                QSFP_CURRENT_LOG,
            ],
        )
