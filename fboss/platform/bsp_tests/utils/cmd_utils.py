import subprocess


def run_cmd(cmd, **kwargs):
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    )
    return result


def check_cmd(cmd, **kwargs):
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    )
    ret_code = result.returncode
    stdout = result.stdout.decode("utf-8")
    stderr = result.stderr.decode("utf-8")

    if ret_code != 0:
        raise Exception(
            f"Command failed: `{' '.join(cmd)}`, stdout={stdout}, stderr={stderr}"
        )
