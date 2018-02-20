from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import paramiko
from scp import SCPClient
from libfb.decorators import memoize_forever
import os

DEFAULT_SSH_TIMEOUT = 5


@memoize_forever
def _get_keyfiles():
    keyfiles = []
    cert_path = ['/var/facebook/credentials/{user}/ssh/id_rsa-cert.pub'.
                 format(user=os.getenv('USER', 'root')),
                 '/root/.ssh/id_rsa-cert.pub']
    for path in cert_path:
        if os.path.isfile(path):
            keyfiles.append(path)
    return keyfiles


class ParamikoClient(object):
    def __init__(self):
        self.client = paramiko.SSHClient()

    def __enter__(self):
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.client.load_system_host_keys()
        return self.client

    def __exit__(self, type, value, traceback):
        self.client.close()


def connect_to_client(client, asset_name, username, password):
    keyfiles = _get_keyfiles()
    # timeout cannot be 0 for connect
    client.connect(hostname=asset_name, username=username,
                   password=password, key_filename=keyfiles,
                   timeout=DEFAULT_SSH_TIMEOUT,
                   allow_agent=False, look_for_keys=False)


def scp_file_to_device(asset_name, username, password, local_path, remote_path):
    with ParamikoClient() as client:
        connect_to_client(client, asset_name, username, password)
        scp = SCPClient(client.get_transport())
        scp.put(local_path, remote_path)


def does_file_exist_on_device(asset_name, username, password, remote_path):
    with ParamikoClient() as client:
        connect_to_client(client, asset_name, username, password)
        sftp = client.open_sftp()
        try:
            sftp.stat(remote_path)
            return True
        except IOError:
            # sftp stat throws IOError when it can noy find a file
            return False


def modify_file_path_on_device(asset_name, username, password,
                               current_file_path, new_file_path):
    with ParamikoClient() as client:
        connect_to_client(client, asset_name, username, password)
        try:
            sftp = client.open_sftp()
            sftp.rename(current_file_path, new_file_path)
        except IOError:
            # stfp rename do not do overwrite when file exist at destination path
            # https://www.unix.com/unix-for-advanced-and-expert-users/
            # 37671-sftp-rename-not-over-writing.html
            # To work around that limitation, we delete file in destination
            # pah and do a copy of file
            sftp.remove(new_file_path)
            sftp.rename(current_file_path, new_file_path)
