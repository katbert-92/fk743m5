#!/usr/bin/env python3

import json
import logging
import os
import re
import sys
from pathlib import Path
from subprocess import PIPE, Popen, TimeoutExpired
from time import time
from typing import Optional, Tuple

import coloredlogs
import toml
from dotenv import load_dotenv
from tqdm import tqdm

# Constants
DEFAULT_LOG_LEVEL = logging.INFO
BUILD_RESULT_DIR = Path("debug")
BUILD_SYSTEM_DIR = Path("build")
BUILD_LOG_DIR = BUILD_RESULT_DIR
MAKE_ERR_LOG_PATH = BUILD_LOG_DIR / "make_err.log"
PYTHON_BUILD_LOG_PATH = BUILD_LOG_DIR / "python_build.log"
FW_VERSIONS = BUILD_SYSTEM_DIR / "fw_versions.toml"
FW_MAIN_INFO = BUILD_RESULT_DIR / "fw_info.json"
ERR_LOG_MAX_LINES = 100

log = logging.getLogger(__name__)


def init_environment() -> logging.Logger:
    """Initializes environment variables, creates directories, and sets up logging"""

    load_dotenv(override=True)
    BUILD_RESULT_DIR.mkdir(parents=True, exist_ok=True)
    MAKE_ERR_LOG_PATH.parent.mkdir(parents=True, exist_ok=True)
    PYTHON_BUILD_LOG_PATH.parent.mkdir(parents=True, exist_ok=True)

    log_level = getattr(logging, os.getenv("LOG_LEVEL", "").upper(), DEFAULT_LOG_LEVEL)
    log_format = "%(asctime)s [%(filename)s:%(lineno)3d] - %(levelname)-7s - %(message)s"

    logger = logging.getLogger()
    logger.setLevel(log_level)

    # Console handler
    coloredlogs.install(level=log_level, logger=logger, fmt=log_format)

    with PYTHON_BUILD_LOG_PATH.open("w+", encoding="utf-8") as f:
        f.write("\n--- New Run Start ---\n")

    # File handler
    file_handler = logging.FileHandler(PYTHON_BUILD_LOG_PATH)
    file_handler.setFormatter(logging.Formatter(log_format))
    file_handler.setLevel(logging.DEBUG)
    logger.addHandler(file_handler)

    return logger


def generate_vscode_launch_from_template() -> None:
    """Generates .vscode/launch.json from a template using environment variables"""

    template_path = Path(".vscode/launch.template.json")
    output_path = Path(".vscode/launch.json")

    if not template_path.exists():
        log.error("Template for .vscode/launch not found")
        return

    def substitute(match: re.Match) -> str:
        key = match.group(1)
        value = os.getenv(key)
        if value is None:
            log.warning(f"Environment variable '{key}' not set")
            return f"<missing:{key}>"
        return value

    content = re.sub(r"\{\{\s*(\w+)\s*\}\}", substitute, template_path.read_text(encoding="utf-8"))
    output_path.write_text(content, encoding="utf-8")
    log.info("VSCode launch.json generated from template")


def get_git_info() -> Tuple[str, str]:
    """Returns the current Git commit hash and branch name"""

    def get_output(cmd: str) -> str:
        return os.popen(cmd).read().strip()

    commit_hash = get_output("git rev-parse --short=6 HEAD")
    branch_name = get_output("git rev-parse --abbrev-ref HEAD")

    if not commit_hash or not branch_name:
        log.warning("Could not get correct git hash or branch")

    return commit_hash, branch_name


def run_make(cmd: str, total_files: Optional[int] = None) -> int:
    """Runs the make command and logs progress using tqdm"""

    log.info(f"Make cmd: {cmd}")

    object_file_regex = re.compile(rf"^{re.escape(str(BUILD_RESULT_DIR))}/(?:[\w\-\.]+/)*\w+\.o$")
    other_str = ""
    bar_fmt = "{l_bar}{bar:80}{r_bar}{bar:-10b}" if total_files else None

    with open(MAKE_ERR_LOG_PATH, "w", encoding="utf-8") as err_f:
        process = Popen(cmd, shell=True, stdout=PIPE, stderr=err_f)

        with tqdm(total=total_files, bar_format=bar_fmt) as pbar:
            while process.poll() is None:
                line = process.stdout.readline()
                if line:
                    line_decoded = line.decode().rstrip()
                    if object_file_regex.match(line_decoded):
                        pbar.set_postfix_str(line_decoded)
                        pbar.update()
                    else:
                        other_str += line_decoded + "\n"

    if not MAKE_ERR_LOG_PATH.exists():
        return process.returncode

    with open(MAKE_ERR_LOG_PATH, "r", encoding="utf-8") as err_f:
        std_err = err_f.readlines()

    if len(std_err) > ERR_LOG_MAX_LINES:
        stderr_message = f"Output is too long ({len(std_err)}), please check {MAKE_ERR_LOG_PATH}"
    elif len(std_err) == 0:
        stderr_message = ""
    else:
        stderr_message = "".join(std_err)

    if stderr_message != "":
        if process.returncode != 0:
            log.error(stderr_message)
        else:
            log.warning(stderr_message)

    if other_str != "":
        with open(PYTHON_BUILD_LOG_PATH, "a", encoding="utf-8") as log_f:
            log_f.write(other_str)

    for file in BUILD_RESULT_DIR.glob("*.bin"):
        log.info(f"Binary file {file}")

    return process.returncode


def build_target(args: list[str], make_path: str, jobs: int) -> int:
    """Builds a specific target using make"""

    args_str = " ".join(args)

    try:
        check_cmd = f"{make_path} -nw {args_str}"
        cdg_cmd = f"{check_cmd} | {sys.executable} {BUILD_SYSTEM_DIR}/cdg.py"
        Popen(cdg_cmd, shell=True).wait(timeout=5)
    except TimeoutExpired:
        pass

    total_files = None
    try:
        with open(f"{BUILD_RESULT_DIR}/compile_commands.json") as f:
            total_files = len(json.load(f))
    except FileNotFoundError:
        log.warning("compile_commands.json not found")

    cmd = f"{make_path} -j{jobs} {args_str}"
    return run_make(cmd, total_files)


def clean_target(args: list[str], make_path: str) -> int:
    """Cleans the build for the specified target"""

    args_str = " ".join(args)

    cmd = f"{make_path} clean"
    return run_make(cmd)


def build_all() -> int:
    """Placeholder for building all targets"""

    log.info("Building all targets... (not implemented)")
    return 0


def elf_size_analyse() -> int:
    """Runs ELF size analysis on all built .elf files"""

    base_cmd = f"{BUILD_SYSTEM_DIR}/elf_size_analyze.py {BUILD_RESULT_DIR}/*.elf --human-readable --no-color --files-only"
    reports = [
        {"key": "-P", "out": "elf_report_sections"},
        {"key": "-R", "out": "elf_report_ram"},
        {"key": "-F", "out": "elf_report_rom"},
    ]

    for report in reports:
        proc = Popen(
            f"{sys.executable} {base_cmd} {report['key']}", shell=True, stdout=PIPE, stderr=PIPE
        )
        stdout, stderr = proc.communicate()

        if stderr:
            log.error(stderr.decode())
        else:
            output_file = f"{BUILD_LOG_DIR}/{report['out']}.log"
            with open(output_file, "w", encoding="utf-8") as f:
                f.write(stdout.decode())
                log.info(f"ELF {report['key']} report in {output_file}")

    return 0


def main() -> None:
    """Main entry point for the script"""
    init_environment()

    if len(sys.argv) < 2:
        log.error("No command provided")
        sys.exit(1)

    generate_vscode_launch_from_template()

    command = sys.argv[1]
    args = sys.argv[2:]
    git_hash, git_branch = get_git_info()

    make_path = os.getenv("MAKE_PATH", "make")
    make_version = os.popen(f"{make_path} --version").read().splitlines()[0]
    log.info(f"Use {make_version} from {make_path}")

    jobs = os.cpu_count() or 8
    log.info(f"Detected {jobs} CPU cores")

    with open(FW_VERSIONS, "r") as f:
        fw_versions = toml.load(f)

    start = time()

    match command:
        case "build":
            type_key = next((p.split("=")[1] for p in args if p.startswith("type=")), None)
            if not type_key:
                log.error("'type' not found in params")
                sys.exit(1)

            version_info = fw_versions["version"].get(type_key, {})
            version_args = [f"{k}={v}" for k, v in version_info.items()]
            git_args = [f"hash={git_hash}", f"branch={git_branch}"]
            log.info(f"Build args extended on: {version_args + git_args}")
            full_args = args + version_args + git_args

            d = {k: v for item in full_args for k, v in [item.split("=", 1)]}
            out_path = Path(FW_MAIN_INFO)
            with out_path.open("w", encoding="utf-8") as f:
                json.dump(d, f, ensure_ascii=False, indent=4)
            log.info(f"Saved fw main info to {out_path}")

            ret = build_target(full_args, make_path, jobs)
        case "clean":
            ret = clean_target(args, make_path)
        case "all":
            ret = build_all()
        case "elf-size-analyse":
            ret = elf_size_analyse()
        case _:
            log.error(f"Unknown command: {command}")
            sys.exit(1)

    log.info(f"Total build time: {time() - start:.1f}s")


if __name__ == "__main__":
    main()
