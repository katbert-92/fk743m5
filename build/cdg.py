#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import re
import sys
from typing import Dict, List

"""
cdg — Compilation Database Generator

Parses output from GNU Make (or Ninja) and generates a `compile_commands.json`
compatible with clang tooling.
"""

FILE_NAME_REGEX = re.compile(r"[\w./+\-]+\.(s|c|cc|cpp|cxx)\b", re.IGNORECASE)
ENTER_DIR_REGEX = re.compile(
    r"^\s*(?:make|ninja)(?:\[\d+\])?: Entering directory [`'\"](?P<dir>.*)[`'\"]\s*$",
    re.MULTILINE,
)
LEAVE_DIR_REGEX = re.compile(
    r"^\s*(?:make|ninja)(?:\[\d+\])?: Leaving directory .*$",
    re.MULTILINE,
)
COMPILERS_REGEX = re.compile(r'\b(g?cc|[gc]\+\+|clang\+?\+?|icecc|s?ccache)(?:.exe)?"?\s')


def parse(make_output: str) -> List[Dict[str, str]]:
    """
    Parse the Make output and extract compiler commands.

    Args:
        make_output (str): Raw string output from `make`.

    Returns:
        List[Dict[str, str]]: Parsed compile commands in clang format.
    """
    results: List[Dict[str, str]] = []
    path_stack: List[str] = []
    pwd: str = ""

    for line in make_output.replace("\r", "").split("\n"):
        line = line.strip()

        enter_match = ENTER_DIR_REGEX.match(line)
        if enter_match:
            pwd = enter_match.group("dir")
            path_stack.append(pwd)
            continue

        if LEAVE_DIR_REGEX.match(line):
            path_stack.pop()
            if path_stack:
                pwd = path_stack[-1]
            continue

        if not COMPILERS_REGEX.search(line):
            continue

        # Trim line to keep only compiler command
        i = COMPILERS_REGEX.search(line).start()
        if line[i:].rstrip().endswith('"'):
            while i > 0 and not (line[i] == '"' and (i == 0 or line[i - 1] != "\\")):
                i -= 1
        else:
            while i > 0 and line[i - 1] not in (" ", "\t", "\n", ";", "&"):
                i -= 1
        line = line[i:].strip()

        file_match = FILE_NAME_REGEX.search(line)
        if not file_match:
            continue

        if not pwd:
            pwd = "/path/to/your/project/"
            path_stack.append(pwd)

        # Strip off anything after `;` or `&&`
        command = line.split(";")[0].split("&&")[0].strip()

        results.append(
            {
                "directory": pwd,
                "file": file_match.group(0),
                "command": command,
            }
        )

    return results


def usage() -> None:
    """Print usage instructions and exit."""
    print(
        f"""Usage: {sys.argv[0]} [compilation-db-file]

[compilation-db-file] is optional (default: debug/compile_commands.json).
Use "-" to output JSON to stdout.

Reads GNU make output from stdin, parses it, and writes JSON compilation database.
"""
    )
    sys.exit(1)


def main() -> None:
    """Main entry point."""
    make_output = sys.stdin.read().strip()
    if not make_output:
        usage()

    db_json = json.dumps(parse(make_output), indent=2) + "\n"

    output_path = sys.argv[1] if len(sys.argv) > 1 else "debug/compile_commands.json"
    if output_path == "-":
        sys.stdout.write(db_json)
    else:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(db_json)


if __name__ == "__main__":
    main()
