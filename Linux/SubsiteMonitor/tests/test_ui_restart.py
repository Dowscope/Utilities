#!/usr/bin/env python3
import fcntl
import os
from pathlib import Path
import pty
import select
import shlex
import signal
import struct
import subprocess
import tempfile
import termios
import time


ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.1.0-alpha.13"


def compile_test_application(binary: Path, bindir: Path, datadir: Path) -> None:
    installer_path = datadir / "installer.sh"
    installed_binary = bindir / "subsite-monitor"
    pkg_cflags = shlex.split(
        subprocess.check_output(
            ["pkg-config", "--cflags", "ncursesw"], text=True
        )
    )
    pkg_libs = shlex.split(
        subprocess.check_output(
            ["pkg-config", "--libs", "ncursesw"], text=True
        )
    )
    sources = sorted((ROOT / "src").glob("*.cpp"))
    command = [
        "g++",
        *pkg_cflags,
        "-Isrc",
        "-std=c++23",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Wconversion",
        f'-DSUBSITE_MONITOR_VERSION="{VERSION}"',
        f'-DSUBSITE_MONITOR_INSTALLER_PATH="{installer_path}"',
        f'-DSUBSITE_MONITOR_EXECUTABLE_PATH="{installed_binary}"',
        *(str(source) for source in sources),
        "-o",
        str(binary),
        *pkg_libs,
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def write_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(0o755)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="subsite-monitor-restart-test-") as temp:
        directory = Path(temp)
        bindir = directory / "bin"
        datadir = directory / "share"
        mock_bin = directory / "mock-bin"
        for path in (bindir, datadir, mock_bin):
            path.mkdir()

        application = directory / "test-application"
        compile_test_application(application, bindir, datadir)

        write_executable(
            datadir / "installer.sh",
            "#!/bin/sh\n"
            "cat >\"$UPDATED_BINARY\" <<'APP'\n"
            "#!/bin/sh\n"
            "printf 'AUTO_RESTART_MARKER\\n'\n"
            "for arg do printf 'RESTART_ARG=%s\\n' \"$arg\"; done\n"
            "APP\n"
            "chmod 0755 \"$UPDATED_BINARY\"\n",
        )
        write_executable(mock_bin / "sudo", '#!/bin/sh\nexec "$@"\n')
        write_executable(
            mock_bin / "python3",
            "#!/bin/sh\n"
            "printf '0.1.0-alpha.14\\t2026-10-30\\t"
            "U3Vic2l0ZSBNb25pdG9yIGFscGhhLjE0\\tTm90ZXM=\\n'\n",
        )

        process_id, terminal = pty.fork()
        if process_id == 0:
            environment = os.environ.copy()
            environment.update(
                {
                    "TERM": "xterm",
                    "PATH": f"{mock_bin}:{environment.get('PATH', '')}",
                    "UPDATED_BINARY": str(bindir / "subsite-monitor"),
                    "SUBSITE_MONITOR_NGINX_CONFIG": str(
                        ROOT / "tests/fixtures/nginx/nginx.conf"
                    ),
                    "SUBSITE_MONITOR_STATS_DIR": str(directory / "stats"),
                }
            )
            os.execve(
                application,
                [
                    str(application),
                    "ui",
                    "--log",
                    str(ROOT / "tests/fixtures/access.log"),
                ],
                environment,
            )

        output = bytearray()

        def wait_for(expected: bytes, timeout: float = 12.0) -> None:
            deadline = time.monotonic() + timeout
            while expected not in output and time.monotonic() < deadline:
                readable, _, _ = select.select([terminal], [], [], 0.2)
                if readable:
                    try:
                        output.extend(os.read(terminal, 8192))
                    except OSError:
                        break
            if expected not in output:
                raise RuntimeError(f"timed out waiting for {expected!r}")

        try:
            fcntl.ioctl(
                terminal,
                termios.TIOCSWINSZ,
                struct.pack("HHHH", 24, 100, 0, 0),
            )
            wait_for(b"SUBSITE MONITOR")
            os.write(terminal, b"u")
            wait_for(b"SOFTWARE UPDATE")
            os.write(terminal, b"\n")
            wait_for(b"Continue? Press y to install")
            os.write(terminal, b"y")
            wait_for(b"AUTO_RESTART_MARKER")
            wait_for(b"RESTART_ARG=--log")
            wait_for(str(ROOT / "tests/fixtures/access.log").encode())
            _, status = os.waitpid(process_id, 0)
            if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
                raise RuntimeError(f"updated process exited with status {status}")
        except Exception:
            try:
                os.kill(process_id, signal.SIGKILL)
            except ProcessLookupError:
                pass
            try:
                os.waitpid(process_id, 0)
            except ChildProcessError:
                pass
            raise
        finally:
            os.close(terminal)

    print("UI update restart tests passed")


if __name__ == "__main__":
    main()
