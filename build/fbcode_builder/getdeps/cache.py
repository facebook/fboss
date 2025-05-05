# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

# pyre-unsafe

import boto3
import botocore
import os
import configparser
from pathlib import Path
from typing import Optional, Tuple


class TransientFailure(Exception):
    """Exception indicating a temporary failure that may succeed on retry"""
    pass


class ArtifactCache(object):
    """The ArtifactCache is a small abstraction that allows caching
    named things in some external storage mechanism.
    The primary use case is for storing the build products on CI
    systems to accelerate the build"""

    def download_to_file(self, name, dest_file_name) -> bool:
        """If `name` exists in the cache, download it and place it
        in the specified `dest_file_name` location on the filesystem.
        If a transient issue was encountered a TransientFailure shall
        be raised.
        If `name` doesn't exist in the cache `False` shall be returned.
        If `dest_file_name` was successfully updated `True` shall be
        returned.
        All other conditions shall raise an appropriate exception."""
        return False

    def upload_from_file(self, name, source_file_name) -> None:
        """Causes `name` to be populated in the cache by uploading
        the contents of `source_file_name` to the storage system.
        If a transient issue was encountered a TransientFailure shall
        be raised.
        If the upload failed for some other reason, an appropriate
        exception shall be raised."""
        pass


class S3ArtifactCache(ArtifactCache):
    def __init__(
        self,
        bucket: str,
        prefix: str = "",
        aws_access_key_id: Optional[str] = None,
        aws_secret_access_key: Optional[str] = None,
        region: Optional[str] = None
    ):
        print(f"CACHE: Initializing S3 client with bucket: {bucket}, prefix: {prefix}")
        self.bucket = bucket
        self.prefix = prefix.rstrip("/") + "/" if prefix else ""

        # Initialize S3 client
        self.s3 = boto3.client(
            's3',
            aws_access_key_id=aws_access_key_id,
            aws_secret_access_key=aws_secret_access_key,
            region_name=region
        )

    def _get_s3_key(self, name: str) -> str:
        """Convert cache name to S3 key"""
        return f"{self.prefix}{name}"

    def download_to_file(self, name: str, dest_file_name: str) -> bool:
        key = self._get_s3_key(name)
        print(f"CACHE: Attempting to download '{key}' from bucket '{self.bucket}' to '{dest_file_name}'")

        try:
            # Ensure directory exists
            os.makedirs(os.path.dirname(dest_file_name), exist_ok=True)

            # Download file from S3
            self.s3.download_file(self.bucket, key, dest_file_name)
            print(f"CACHE: Successfully downloaded '{key}' to '{dest_file_name}'")
            return True

        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] == '404':
                print(f"CACHE: Object '{key}' not found in cache")
                return False
            elif e.response['Error']['Code'] in ['SlowDown', 'ServiceUnavailable']:
                print(f"CACHE: Transient S3 error while downloading '{key}': {str(e)}")
                raise TransientFailure(f"Transient S3 error: {str(e)}")
            else:
                print(f"CACHE: Error downloading '{key}': {str(e)}")
                raise Exception(f"S3 error downloading {name}: {str(e)}")

        except (botocore.exceptions.BotoCoreError,
                botocore.exceptions.ConnectionError) as e:
            print(f"CACHE: Connection error while downloading '{key}': {str(e)}")
            raise TransientFailure(f"Transient S3 connection error: {str(e)}")

    def upload_from_file(self, name: str, source_file_name: str) -> None:
        key = self._get_s3_key(name)
        print(f"CACHE: Attempting to upload '{source_file_name}' to '{key}' in bucket '{self.bucket}'")

        try:
            self.s3.upload_file(source_file_name, self.bucket, key)
            print(f"CACHE: Successfully uploaded '{source_file_name}' to '{key}'")

        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] in ['SlowDown', 'ServiceUnavailable']:
                print(f"CACHE: Transient S3 error while uploading '{key}': {str(e)}")
                raise TransientFailure(f"Transient S3 error: {str(e)}")
            else:
                print(f"CACHE: Error uploading '{key}': {str(e)}")
                raise Exception(f"S3 error uploading {name}: {str(e)}")

        except (botocore.exceptions.BotoCoreError,
                botocore.exceptions.ConnectionError) as e:
            print(f"CACHE: Connection error while uploading '{key}': {str(e)}")
            raise TransientFailure(f"Transient S3 connection error: {str(e)}")


def create_cache(
    cache_config: Optional[str] = None,
) -> ArtifactCache:
    """Create an ArtifactCache instance based on the provided configuration
    file."""
    if not cache_config:
        return ArtifactCache()

    config = configparser.ConfigParser()
    with open(cache_config) as f:
        config.read_file(f)

    if "s3" in config:
        return S3ArtifactCache(
            bucket=config["s3"]["bucket"],
            prefix=config["s3"].get("prefix", ""),
            aws_access_key_id=config["s3"].get("aws_access_key_id"),
            aws_secret_access_key=config["s3"].get("aws_secret_access_key"),
            region=config["s3"].get("region")
        )
    else:
        print(f"CACHE: No supported cache configuration found in {cache_config}")
        return ArtifactCache()
