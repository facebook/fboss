import typing as t
from datetime import datetime, timedelta

import fboss.lib.link_snapshots.snapshot_lib as snapshot_lib
from fboss.lib.link_snapshots.snapshot_lib import SNAPSHOT_FORMAT, Backend
from later.unittest.mock import AsyncContextManager, AsyncMock, patch, Mock
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
    async def mock_snapshots(self, mock_get_rfe_client, mock_ssh_client):
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

        # mock scuba
        rfe_client = AsyncMock()
        rfe_client.querySQL.return_value = rfe.SQLQueryResult(
            value=[[snapshot_line] for snapshot_line in snapshot_lines]
        )
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=rfe_client, instance=True
        )

        # mock ssh

        mock_stdout = Mock()
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stdout.read.return_value = "\n".join(snapshot_lines).encode()
        mock_ssh_client.return_value.exec_command.return_value = [
            Mock(),
            mock_stdout,
            Mock(),
        ]

        return await snapshot_lib.get_client()

    @patch("fboss.lib.link_snapshots.snapshot_lib.paramiko.SSHClient")
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_recent_snapshots(self, mock_get_rfe_client, mock_ssh_client):
        def check_snapshots(snapshots: t.Mapping[int, t.Any]):
            for i in range(NUM_SNAPSHOTS):
                self.assertTrue(i in snapshots)
                snapshot = snapshots[i]
                self.assertTrue(snapshot is PhyInfo or snapshot is TransceiverInfo)
                self.assertEqual(snapshot.timeCollected, i)

        client = await self.mock_snapshots(mock_get_rfe_client, mock_ssh_client)

        # Test scuba

        # We're using fetch_recent_snapshots for simplicity. We dont go to an
        # actual SQL engine so we never check the timestamps in these tests
        snapshot_collection = await client.fetch_recent_snapshots(
            HOSTNAME, PORT_NAME, backend=Backend.SCUBA
        )
        iphy_snapshots, xphy_snapshots, tcvr_snapshots = snapshot_collection.unpack()

        self.assertEqual(len(iphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(xphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(tcvr_snapshots), NUM_SNAPSHOTS)

        # test ssh
        snapshot_collection = await client.fetch_recent_snapshots(
            HOSTNAME, PORT_NAME, backend=Backend.SSH
        )
        iphy_snapshots, xphy_snapshots, tcvr_snapshots = snapshot_collection.unpack()

        self.assertEqual(len(iphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(xphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(tcvr_snapshots), NUM_SNAPSHOTS)

    @patch("fboss.lib.link_snapshots.snapshot_lib.paramiko.SSHClient")
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_around_time(
        self, mock_get_rfe_client, mock_ssh_client
    ):
        client = await self.mock_snapshots(mock_get_rfe_client, mock_ssh_client)

        # via scuba

        # test default param
        await client.fetch_snapshots_around_time(
            HOSTNAME, PORT_NAME, datetime.now(), backend=Backend.SCUBA
        )

        # test non-default param
        await client.fetch_snapshots_around_time(
            HOSTNAME,
            PORT_NAME,
            datetime.now(),
            timedelta(seconds=1),
            backend=Backend.SCUBA,
        )

        # via ssh

        # test default param
        await client.fetch_snapshots_around_time(
            HOSTNAME, PORT_NAME, datetime.now(), backend=Backend.SSH
        )

        # test non-default param
        await client.fetch_snapshots_around_time(
            HOSTNAME,
            PORT_NAME,
            datetime.now(),
            timedelta(seconds=1),
            backend=Backend.SSH,
        )

    @patch("fboss.lib.link_snapshots.snapshot_lib.paramiko.SSHClient")
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_in_timespan(
        self, mock_get_rfe_client, mock_ssh_client
    ):
        client = await self.mock_snapshots(mock_get_rfe_client, mock_ssh_client)

        await client.fetch_snapshots_in_timespan(
            HOSTNAME, PORT_NAME, datetime.now(), datetime.now(), backend=Backend.SCUBA
        )

        await client.fetch_snapshots_in_timespan(
            HOSTNAME, PORT_NAME, datetime.now(), datetime.now(), backend=Backend.SSH
        )
