import io
import logging
import os
import re
import sys

import coloredlogs
from elftools.elf.elffile import ELFFile

DEFAULT_LOG_LEVEL = logging.INFO
log_level = getattr(logging, os.getenv("LOG_LEVEL", "").upper(), DEFAULT_LOG_LEVEL)
log_format = "%(asctime)s [%(filename)17s:%(lineno)3d] - %(levelname)-7s - %(message)s"

logger = logging.getLogger()
logger.setLevel(log_level)

coloredlogs.install(level=log_level, logger=logger, fmt=log_format)


class MapParser:
    def __init__(self, map_file_path: str) -> None:
        self.map_file_path = map_file_path
        self.map_lines = []
        self._load_map_data()

    def _load_map_data(self) -> None:
        logger.info(f"Loading map file: {self.map_file_path}")
        try:
            with open(self.map_file_path, "r") as f:
                self.map_lines = f.readlines()
            logger.info(f"File {self.map_file_path} successfully loaded")
        except FileNotFoundError:
            logger.error(f"Map file not found: {self.map_file_path}")
            sys.exit(1)
        except IOError as e:
            logger.error(f"Error reading file {self.map_file_path}: {e}")
            sys.exit(1)

    def get_regions_info(self) -> dict:
        if not self.map_lines:
            logger.warning("Map data is empty")
            return {}

        section_re = re.compile(r"^(\S+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+\S+")
        memory_regions = {}

        header_index = None
        for i, line in enumerate(self.map_lines):
            if line.strip().startswith("Name") and "Origin" in line and "Length" in line:
                header_index = i
                break

        if header_index is None:
            logger.warning("Memory region section not found in map file")
            return {}

        for line in self.map_lines[header_index + 1 :]:
            if line.strip() == "" or line.strip().startswith("*default*"):
                continue

            match = section_re.match(line)
            if match:
                name, origin_str, length_str = match.groups()
                origin = int(origin_str, 16)
                length = int(length_str, 16)
                memory_regions[name] = {"origin": origin, "length": length}
                logger.debug(f"Parsed region: {name}, Origin: {origin_str}, Length: {length_str}")
            else:
                break

        logger.info(f"Parsed {len(memory_regions)} memory regions")
        return memory_regions


class ElfParser:
    def __init__(self, elf_path: str) -> None:
        self.elf_path = elf_path
        self.elf_data = None
        self.elf = None
        self._load_elf_data()

    def _load_elf_data(self) -> None:
        logger.info(f"Loading ELF file: {self.elf_path}")
        try:
            with open(self.elf_path, "rb") as f:
                self.elf_data = f.read()
            self.elf = ELFFile(io.BytesIO(self.elf_data))
            logger.info(f"File {self.elf_path} successfully loaded")
        except FileNotFoundError:
            logger.error(f"File {self.elf_path} not found")
            sys.exit(1)
        except IOError as e:
            logger.error(f"Error reading file {self.elf_path}: {e}")
            sys.exit(1)

    def get_sections_info(self) -> dict:
        if not self.elf:
            logger.error("ELF data not loaded properly")
            return {}

        section_info = {}
        for section in self.elf.iter_sections():
            sec_name = section.name
            sec_header = section.header
            section_info[sec_name] = dict(sec_header)

        logger.info(f"Extracted info from {len(section_info)} sections")
        return section_info

    def get_symbol_sizes(self) -> dict:
        if not self.elf:
            logger.error("ELF data not loaded properly")
            return {}

        function_sizes = {}
        variable_sizes = {}
        other_sizes = {}

        for section in self.elf.iter_sections():
            if section.header["sh_type"] == "SHT_SYMTAB":
                for symbol in section.iter_symbols():
                    if symbol.entry.st_size > 0:
                        symbol_type = symbol.entry["st_info"]["type"]

                        if symbol_type == "STT_FUNC":
                            function_sizes[symbol.name] = symbol.entry.st_size
                        elif symbol_type == "STT_OBJECT":
                            variable_sizes[symbol.name] = symbol.entry.st_size
                        else:
                            other_sizes[symbol.name] = symbol.entry.st_size

        logger.info(f"Collected {len(function_sizes)} functions, {len(variable_sizes)} variables")
        return {"functions": function_sizes, "variables": variable_sizes, "others": other_sizes}

    def get_function_addresses(self) -> dict:
        if not self.elf:
            logger.error("ELF data not loaded properly")
            return {}

        function_addresses = {}
        for section in self.elf.iter_sections():
            if section.header["sh_type"] == "SHT_SYMTAB":
                for symbol in section.iter_symbols():
                    if symbol.entry.st_info["type"] == "STT_FUNC":
                        function_addresses[symbol.name] = symbol.entry.st_value

        logger.info(f"Found {len(function_addresses)} function addresses")
        return function_addresses
