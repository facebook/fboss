# pyre-unsafe

from fboss.util.fixmyfboss.check import check
from fboss.util.fixmyfboss.status import Problem


@check
def expected_kernel_version():
    """
    Check that we are running a stable image
    """
    supported_versions = ["6.4.3-0_fbk1_755_ga25447393a1d"]
    # run `uname -r` to get the current kernel version
    import subprocess

    # Run `uname -r` to get the current kernel version
    current_kernel_version = subprocess.check_output(["uname", "-r"]).decode().strip()

    if current_kernel_version not in supported_versions:
        return Problem(
            description=f"""This device is running a non-supported kernel version:
                Current: {current_kernel_version}
                Expected: {", ".join(supported_versions)}""",
            manual_remediation="""Please reprovision the device""",
        )
