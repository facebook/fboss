#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import os
import shutil
import signal
import subprocess

import git

"""
   Helper script to help with FBOSS builds that link with SAI.

   Input:
      Path to SAI implementation library libsai_impl.a
      Path to SAI implementation experimental directory.
      Path to output directory

   Output:
      Organizes input files into the desired directory structure.
      Creates a tarball.
      Creates a manifest/sai_impl for this tarball.
      Copies the sai_impl at the manifest location.
      Edit fboss manifest to add sai_impl dependency.
      Starts local http.server: for gerdeps to download libsai.

   Sample invocation:

      cd $HOME/fboss.git/installer/centos-8-x86_64
      ./build-helper.py $HOME/brcm-sai/build/lib/libsai_impl.a $HOME/brcm-sai/headers/experimental/ $HOME/sai_impl_output

   After the above setup, one could simply start FBOSS build e.g.:

   export SAI_ONLY=1
   export SAI_BRCM_IMPL=1
   ./build/fbcode_builder/getdeps.py build --allow-system-packages fboss
"""


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("libsai_impl_path", help="Full path to libsai_impl.a")
    parser.add_argument(
        "experiments_path", help="Full path to SAI spec experiments dir"
    )
    parser.add_argument("output_path", help="Output dir")

    return parser.parse_args()


class BuildHelper:
    LIBSAI_IMPL_COMPRESSED_TAR = "libsai_impl.tar.gz"

    def __init__(self, args):
        self._script_dir = os.getcwd()
        self._working_tree_dir = git.Repo(
            ".", search_parent_directories=True
        ).working_tree_dir
        self._fboss_manifest_path = os.path.join(
            self._working_tree_dir, "build/fbcode_builder/manifests/fboss"
        )
        self._sai_impl_manifest_path = os.path.join(
            self._working_tree_dir, "build/fbcode_builder/manifests/sai_impl"
        )
        self._libsai_manifest_path = os.path.join(
            self._working_tree_dir, "build/fbcode_builder/manifests/libsai"
        )
        self._libsai_impl_path = args.libsai_impl_path
        self._experiments_path = args.experiments_path
        self._output_path = args.output_path

    def _kill_http_server(self):
        try:
            http_pid = (
                subprocess.check_output(["lsof", "-i", ":8000"])
                .decode("utf-8")
                .strip()
                .split("\n")[1]
                .split()[1]
            )
            os.kill(int(http_pid), signal.SIGTERM)
        except subprocess.CalledProcessError:
            pass

    def _remove_output_dir(self):
        if os.path.isdir(self._output_path):
            shutil.rmtree(self._output_path)

    def _cleanup(self):
        self._kill_http_server()
        self._remove_output_dir()

    def _copy_input_files(self):
        print(f"Copying files to {self._output_path}")
        os.makedirs(self._output_path)
        lib_path = os.path.join(self._output_path, "lib")
        os.makedirs(os.path.join(self._output_path, lib_path))
        headers_path = os.path.join(self._output_path, "include")
        shutil.copy(self._libsai_impl_path, lib_path)
        shutil.copytree(self._experiments_path, headers_path)

    def _create_archive(self):
        subprocess.run(
            [
                "tar",
                "czvf",
                os.path.join(self._output_path, BuildHelper.LIBSAI_IMPL_COMPRESSED_TAR),
                "-C",
                self._output_path,
                "lib",
                "include",
            ]
        )

    def _get_csum(self):
        return (
            subprocess.check_output(
                [
                    "sha256sum",
                    os.path.join(
                        self._output_path, BuildHelper.LIBSAI_IMPL_COMPRESSED_TAR
                    ),
                ]
            )
            .decode("utf-8")
            .strip()
            .split()[0]
        )

    def _edit_sai_manifest(self):
        manifest_str = (
            "[manifest]\n"
            "name = libsai\n"
            "\n"
            "[download]\n"
            "url = https://github.com/opencomputeproject/SAI/archive/v1.14.0.tar.gz\n"
            "sha256 = 4e3a1d010bda0c589db46e077725a2cd9624a5cc255c89d1caa79deb408d1fa7\n"
            "\n"
            "[build]\n"
            "builder = nop\n"
            "subdir = SAI-1.14.0\n"
            "\n"
            "[install.files]\n"
            "inc = include\n"
        )

        with open(self._libsai_manifest_path, "w+") as f:
            f.writelines(manifest_str)

    def _edit_sai_impl_manifest(self):
        csum = self._get_csum()

        manifest_str = (
            "[manifest]\n"
            "name = sai_impl\n"
            "\n"
            "[download]\n"
            "url = http://localhost:8000/libsai_impl.tar.gz\n"
            f"sha256 = {csum}\n"
            "[build]\n"
            "builder = nop\n"
            "\n"
            "[install.files]\n"
            "lib = lib\n"
            "include = include\n"
        )

        with open(self._sai_impl_manifest_path, "w+") as f:
            f.writelines(manifest_str)

    def _edit_fboss_manifest(self):
        with open(self._fboss_manifest_path, encoding="utf-8") as f:
            data = f.readlines()

        # sai_impl dependency is already added to the fboss manifest
        if "sai_impl\n" in data:
            return

        with open(self._fboss_manifest_path, "w", encoding="utf-8") as f:
            for d in data:
                f.write(d)
                if "[dependencies]" in d:
                    f.write("sai_impl\n")

    def _start_http_server(self):
        os.chdir(self._output_path)
        subprocess.Popen(["python3", "-m", "http.server"])
        os.chdir(self._script_dir)

    def run(self):
        self._cleanup()
        self._copy_input_files()
        self._create_archive()
        self._edit_sai_manifest()
        self._edit_sai_impl_manifest()
        self._edit_fboss_manifest()
        self._start_http_server()


if __name__ == "__main__":
    args = parse_args()
    BuildHelper(args).run()
